/***************************************************************************
 *   Copyright (C) 2020-2025 by Stefan Kebekus                             *
 *   stefan.kebekus@gmail.com                                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QGuiApplication>
#include <QLockFile>
#include <QSaveFile>
#include <QNetworkReply>

#include "sunset.h"

#include "navigation/Clock.h"
#include "navigation/Navigator.h"
#include "positioning/PositionProvider.h"
#include "weather/WeatherDataProvider.h"

using namespace std::chrono_literals;
using namespace Qt::Literals::StringLiterals;


Weather::WeatherDataProvider::WeatherDataProvider(QObject *parent) : QObject(parent)
{
    QTimer::singleShot(0, this, &Weather::WeatherDataProvider::deferredInitialization);
}


void Weather::WeatherDataProvider::deferredInitialization()
{
    // Read METAR/TAF from "weather.dat".
    bool const success = load();

    // Request METAR/TAF data every updateIntervalNormal_ms
    connect(&m_updateTimer, &QTimer::timeout, this, &Weather::WeatherDataProvider::requestUpdate);
    m_updateTimer.setInterval(updateIntervalNormal_ms);
    m_updateTimer.start();

    // Check for METAR/TAF updates in 15s, when the flight route changes, the app becomes active, and when the PositionProvider starts
    // receiving data.
    QTimer::singleShot(15s, this, &Weather::WeatherDataProvider::requestUpdate);
    connect(GlobalObject::navigator()->flightRoute(), &Navigation::FlightRoute::waypointsChanged, this, &Weather::WeatherDataProvider::requestUpdate);
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, [this](Qt::ApplicationState state) {
        if ((state | Qt::ApplicationActive) != 0)
        {
            QTimer::singleShot(0, this, &Weather::WeatherDataProvider::requestUpdate);
        }
    });
    connect(GlobalObject::positionProvider(), &Positioning::PositionProvider::receivingPositionInfoChanged, [this](bool rcv) {
        if (rcv)
        {
            QTimer::singleShot(0, this, &Weather::WeatherDataProvider::requestUpdate);
        }
    });

    // Check every 10min for expired messages
    connect(&m_deleteExiredMessagesTimer, &QTimer::timeout, this, &Weather::WeatherDataProvider::deleteExpiredMesages);
    m_deleteExiredMessagesTimer.setInterval(10min);
    m_deleteExiredMessagesTimer.start();

    // Announce updates in QNHInfo and SunInfo once per minute.
    connect(Navigation::Navigator::clock(), &Navigation::Clock::timeChanged, this, &Weather::WeatherDataProvider::QNHInfoChanged);
    connect(Navigation::Navigator::clock(), &Navigation::Clock::timeChanged, this, &Weather::WeatherDataProvider::sunInfoChanged);
}


void Weather::WeatherDataProvider::deleteExpiredMesages()
{
    {
        auto tmpMETARs = m_METARs.value();
        tmpMETARs.removeIf([](const std::pair<const QString&, METAR&>& pair) {return !pair.second.isValid();});
        tmpMETARs.removeIf([](const std::pair<const QString&, METAR&>& pair) {return pair.second.expiration() < QDateTime::currentDateTime();});
        if (tmpMETARs.size() < m_METARs.value().size())
        {
            m_METARs = tmpMETARs;
        }
    }

    {
        auto tmpTAFs = m_TAFs.value();
        tmpTAFs.removeIf([](const std::pair<const QString&, TAF&>& pair) {return !pair.second.isValid();});
        tmpTAFs.removeIf([](const std::pair<const QString&, TAF&>& pair) {return pair.second.expiration() < QDateTime::currentDateTime();});
        if (tmpTAFs.size() < m_TAFs.value().size())
        {
            m_TAFs = tmpTAFs;
        }
    }

    save();
}


bool Weather::WeatherDataProvider::downloading() const
{
    for(const auto& networkReply : m_networkReplies)
    {
        if (networkReply.isNull())
        {
            continue;
        }
        if (networkReply->isRunning())
        {
            return true;
        }
    }

    return false;
}


void Weather::WeatherDataProvider::downloadFinished()
{
    // Start to process the data only once ALL replies have been received. So, we check here if there are any running
    // download processes and abort if indeed there are some.
    if (downloading())
    {
        return;
    }

    // Update flag
    emit downloadingChanged();

    // Read all replies and store the data in respective maps
    bool hasError = false;
    for(const auto& networkReply : m_networkReplies)
    {
        // Paranoid safety checks
        if (networkReply.isNull())
        {
            continue;
        }
        if (networkReply->error() != QNetworkReply::NoError)
        {
            hasError = true;
            emit error(networkReply->errorString());
            continue;
        }

        // Decode XML
        QXmlStreamReader xml(networkReply);
        QList<Weather::METAR> newMETARs;
        QList<Weather::TAF> newTAFs;
        while (!xml.atEnd() && !xml.hasError())
        {
            xml.readNext();
            if(xml.hasError())
            {
                hasError = true;
                break;
            }

            // Read METAR
            if (xml.isStartElement() && (xml.name() == QStringLiteral("METAR")))
            {
                Weather::METAR const metar(xml);
                if (metar.isValid())
                {
                    newMETARs << metar;
                }
            }

            // Read TAF
            if (xml.isStartElement() && (xml.name() == QStringLiteral("TAF")))
            {
                Weather::TAF const taf(xml);
                if (taf.isValid())
                {
                    newTAFs << taf;
                }
            }
        }

        auto tmpMETARs = m_METARs.value();
        for(const auto& newMETAR : newMETARs)
        {
            tmpMETARs[newMETAR.ICAOCode()] = newMETAR;
        }
        m_METARs = tmpMETARs;

        auto tmpTAFs = m_TAFs.value();
        for(const auto& newTAF : newTAFs)
        {
            tmpTAFs[newTAF.ICAOCode()] = newTAF;
        }
        m_TAFs = tmpTAFs;

        updateLogEntry le = {QDateTime::currentDateTimeUtc(), networkReply->property("bBox").value<QGeoRectangle>()};
        if (le.m_bBox.isValid() && le.m_time.isValid())
        {
            updateLog << le;
        }
        m_updateTimer.setInterval(updateIntervalNormal_ms);
    }

    // Clear replies container
    foreach(auto networkReply, m_networkReplies)
    {
        // Paranoid safety checks
        if (!networkReply.isNull())
        {
            networkReply->deleteLater();
        }
    }
    m_networkReplies.clear();

    // Update flag and signals
    emit QNHInfoChanged();

    if (hasError)
    {
        m_updateTimer.setInterval(updateIntervalOnError_ms);
    }
    else
    {
        save();
    }
}


bool Weather::WeatherDataProvider::load()
{
    auto stdFileName = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/weather.dat";

    // Use LockFile. If lock could not be obtained, do nothing.
    QLockFile lockFile(stdFileName+".lock");
    if (!lockFile.tryLock())
    {
        return false;
    }

    // Open file
    auto inputFile = QFile(stdFileName);
    if (!inputFile.open(QIODevice::ReadOnly))
    {
        lockFile.unlock();
        return false;
    }

    // Generate input stream
    QDataStream inputStream(&inputFile);
    inputStream.setVersion(QDataStream::Qt_4_0);
    // Check magic number and version
    quint32 magic = 0;
    inputStream >> magic;
    if (magic != static_cast<quint32>(0x31415))
    {
        lockFile.unlock();
        return false;
    }
    quint32 version = 0;
    inputStream >> version;
    if (version != static_cast<quint32>(1))
    {
        lockFile.unlock();
        return false;
    }

    // Read time of last update
    inputStream >> updateLog;

    // Read file
    QMap<QString, Weather::METAR> newMETARs;
    inputStream >> newMETARs;
    m_METARs = newMETARs;

    QMap<QString, Weather::TAF> newTAFs;
    inputStream >> newTAFs;
    m_TAFs = newTAFs;

    // Ok, done
    lockFile.unlock();
    deleteExpiredMesages();

    return (inputStream.status() == QDataStream::Ok);
}


void Weather::WeatherDataProvider::save()
{
    auto stdFileName = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/weather.dat";

    // Use LockFile. If lock could not be obtained, do nothing.
    QLockFile lockFile(stdFileName+".lock");
    if (!lockFile.tryLock())
    {
        return;
    }

    // Open file
    auto outputFile = QSaveFile(stdFileName);
    if (!outputFile.open(QIODevice::WriteOnly))
    {
        lockFile.unlock();
        return;
    }

    // Generate output stream
    QDataStream outputStream(&outputFile);
    outputStream.setVersion(QDataStream::Qt_4_0);

    // Write magic number and version
    outputStream << static_cast<quint32>(0x31415);
    outputStream << static_cast<quint32>(1);
    outputStream << updateLog;

    outputStream << m_METARs.value();
    outputStream << m_TAFs.value();

    outputFile.commit();
    lockFile.unlock();
}


QString Weather::WeatherDataProvider::sunInfo()
{
    // Paranoid safety checks
    auto *positionProvider = GlobalObject::positionProvider();
    if (positionProvider == nullptr)
    {
        return {};
    }
    if (!positionProvider->positionInfo().isValid())
    {
        return tr("Waiting for precise position…");
    }

    // Describe next sunset/sunrise
    QDateTime sunrise;
    QDateTime sunset;
    QDateTime sunriseTomorrow;

    SunSet sun;
    auto coord = positionProvider->positionInfo().coordinate();
    auto timeZone = qRound(coord.longitude()/15.0);

    auto currentTime = QDateTime::currentDateTimeUtc();
    auto localTime = currentTime.toOffsetFromUtc(timeZone*60*60);
    auto localDate = localTime.date();

    sun.setPosition(coord.latitude(), coord.longitude(), timeZone);
    sun.setCurrentDate(localDate.year(), localDate.month(), localDate.day());

    auto sunriseTimeInMin = sun.calcSunrise();
    if (qIsFinite(sunriseTimeInMin))
    {
        sunrise = localTime;
        sunrise.setTime(QTime::fromMSecsSinceStartOfDay(qRound(sunriseTimeInMin*60*1000)));
        sunrise = sunrise.toOffsetFromUtc(0);
        sunrise.setTimeZone(QTimeZone(Qt::UTC));
    }

    auto sunsetTimeInMin = sun.calcSunset();
    if (qIsFinite(sunsetTimeInMin))
    {
        sunset = localTime;
        sunset.setTime(QTime::fromMSecsSinceStartOfDay(qRound(sunsetTimeInMin*60*1000)));
        sunset = sunset.toOffsetFromUtc(0);
        sunset.setTimeZone(QTimeZone(Qt::UTC));
    }

    localTime = localTime.addDays(1);
    localDate = localTime.date();
    sun.setCurrentDate(localDate.year(), localDate.month(), localDate.day());
    auto sunriseTomorrowTimeInMin = sun.calcSunrise();
    if (qIsFinite(sunriseTomorrowTimeInMin))
    {
        sunriseTomorrow = localTime;
        sunriseTomorrow.setTime(QTime::fromMSecsSinceStartOfDay(qRound(sunriseTomorrowTimeInMin*60*1000)));
        sunriseTomorrow = sunriseTomorrow.toOffsetFromUtc(0);
        sunriseTomorrow.setTimeZone(QTimeZone(Qt::UTC));
    }

    if (sunrise.isValid() && sunset.isValid() && sunriseTomorrow.isValid())
    {
        if (currentTime < sunrise)
        {
            return tr("SR %1, %2").arg(Navigation::Clock::describePointInTime(sunrise), Navigation::Clock::describeTimeDifference(sunrise));
        }
        if (currentTime < sunset.addSecs(40LL*60LL))
        {
            return tr("SS %1, %2").arg(Navigation::Clock::describePointInTime(sunset), Navigation::Clock::describeTimeDifference(sunset));
        }
        return tr("SR %1, %2").arg(Navigation::Clock::describePointInTime(sunriseTomorrow), Navigation::Clock::describeTimeDifference(sunriseTomorrow));
    }
    return {};
}


Units::Pressure Weather::WeatherDataProvider::QNH() const
{
    // Paranoid safety checks
    auto *positionProvider = GlobalObject::positionProvider();
    if (positionProvider == nullptr)
    {
        return {};
    }

    // Find QNH of nearest airfield
    METAR closestMETARWithQNH;
    for (auto i = m_METARs.value().cbegin(), end = m_METARs.value().cend(); i != end; ++i)
    {
        if (!i.value().isValid())
        {
            continue;
        }
        if (!i.value().QNH().isFinite())
        {
            continue;
        }
        if (!i.value().coordinate().isValid())
        {
            continue;
        }
        if (!closestMETARWithQNH.isValid()) {
            closestMETARWithQNH = i.value();
            continue;
        }

        QGeoCoordinate const here = Positioning::PositionProvider::lastValidCoordinate();
        if (here.distanceTo(i.value().coordinate()) < here.distanceTo(closestMETARWithQNH.coordinate()))
        {
            closestMETARWithQNH = i.value();
        }
    }
    if (closestMETARWithQNH.isValid())
    {
        return closestMETARWithQNH.QNH();
    }
    return {};
}


QString Weather::WeatherDataProvider::QNHInfo() const
{
    // Paranoid safety checks
    auto *positionProvider = GlobalObject::positionProvider();
    if (positionProvider == nullptr)
    {
        return {};
    }

    // Find QNH of nearest airfield
    METAR closestMETARWithQNH;
    for (auto i = m_METARs.value().cbegin(), end = m_METARs.value().cend(); i != end; ++i)
    {
        if (!i.value().isValid())
        {
            continue;
        }
        if (!i.value().QNH().isFinite())
        {
            continue;
        }
        if (!i.value().coordinate().isValid())
        {
            continue;
        }
        if (!closestMETARWithQNH.isValid()) {
            closestMETARWithQNH = i.value();
            continue;
        }

        QGeoCoordinate const here = Positioning::PositionProvider::lastValidCoordinate();
        if (here.distanceTo(i.value().coordinate()) < here.distanceTo(closestMETARWithQNH.coordinate()))
        {
            closestMETARWithQNH = i.value();
        }
    }

    if (closestMETARWithQNH.isValid())
    {
        return tr("%1 hPa in %2, %3").arg(qRound(closestMETARWithQNH.QNH().toHPa()))
        .arg(closestMETARWithQNH.ICAOCode(),
             Navigation::Clock::describeTimeDifference(closestMETARWithQNH.observationTime()));
    }
    return {};
}


void Weather::WeatherDataProvider::requestUpdate()
{
    // Generate queries
    const QGeoCoordinate& position = Positioning::PositionProvider::lastValidCoordinate();
    auto steerpts = GlobalObject::navigator()->flightRoute()->geoPath();
    if (position.isValid())
    {
        steerpts.prepend(position);
    }
    QGeoRectangle bBox(steerpts);
    if (!bBox.isValid())
    {
        return;
    }

    // If bBox is crazy large, reduce its size by setting it centered around the current position
    auto diagonal = Units::Distance::fromM(bBox.topLeft().distanceTo(bBox.bottomRight()));
    if (diagonal > Units::Distance::fromNM(500))
    {
        bBox = QGeoRectangle({position});
    }

    bBox.setHeight( bBox.height()+2.0 );
    auto factor =  cos(qDegreesToRadians( qMin(80.0, qAbs(bBox.center().latitude())) ));
    bBox.setWidth( bBox.width() + 2.0/factor );

    startDownload(bBox);
}


void Weather::WeatherDataProvider::requestUpdate4Waypoint(const GeoMaps::Waypoint& wp)
{
    if (!wp.coordinate().isValid() || (wp.ICAOCode().length() < 4))
    {
        return;
    }
    startDownload(QGeoRectangle(wp.coordinate(), 2, 2));
}


void Weather::WeatherDataProvider::startDownload(const QGeoRectangle& _bBox)
{
    if (!_bBox.isValid())
    {
        return;
    }

    // Round bounding box, so coordinates become integers
    QGeoRectangle bBox;
    bBox.setBottomLeft({std::floor(_bBox.bottomLeft().latitude()), std::floor(_bBox.bottomLeft().longitude())});
    bBox.setTopRight({std::ceil(_bBox.topRight().latitude()), std::ceil(_bBox.topRight().longitude())});

    // Check if bounding box is already covered
    updateLog.removeIf([](const updateLogEntry& ule) {return !ule.m_bBox.isValid() || !ule.m_time.isValid();});
    updateLog.removeIf([](const updateLogEntry& ule) {return ule.m_time.addSecs(5*60) < QDateTime::currentDateTimeUtc();});
    for(const auto& ule : updateLog)
    {
        if (ule.m_bBox.contains(bBox))
        {
            return;
        }
    }
    m_networkReplies.removeAll(nullptr);
    for(const auto& nwr : m_networkReplies)
    {
        if (!nwr->isRunning())
        {
            continue;
        }
        if (nwr->property("area").value<QGeoRectangle>().contains(bBox))
        {
            return;
        }
    }

    {
        QString const urlString
            = u"https://enroute-data.akaflieg-freiburg.de/enrouteProxy/metar.php?format=xml&bbox=%1,%2,%3,%4"_s
                  .arg(bBox.bottomLeft().latitude())
                  .arg(bBox.bottomLeft().longitude())
                  .arg(bBox.topRight().latitude())
                  .arg(bBox.topRight().longitude());
        QUrl const url = QUrl(urlString);
        QNetworkRequest request(url);
        request.setRawHeader("accept", "application/xml");
        request.setTransferTimeout(2min);
        QPointer<QNetworkReply> const reply = GlobalObject::networkAccessManager()->get(request);
        reply->setProperty("bBox", QVariant::fromValue(bBox));
        m_networkReplies.push_back(reply);
        connect(reply, &QNetworkReply::finished, this, &Weather::WeatherDataProvider::downloadFinished);
        connect(reply, &QNetworkReply::errorOccurred, this, &Weather::WeatherDataProvider::downloadFinished);
    }

    {
        QString const urlString
            = u"https://enroute-data.akaflieg-freiburg.de/enrouteProxy/taf.php?format=xml&bbox=%1,%2,%3,%4"_s
                  .arg(bBox.bottomLeft().latitude())
                  .arg(bBox.bottomLeft().longitude())
                  .arg(bBox.topRight().latitude())
                  .arg(bBox.topRight().longitude());
        QUrl const url = QUrl(urlString);
        QNetworkRequest request(url);
        request.setRawHeader("accept", "application/xml");
        request.setTransferTimeout(2min);
        QPointer<QNetworkReply> const reply = GlobalObject::networkAccessManager()->get(request);

        m_networkReplies.push_back(reply);
        connect(reply, &QNetworkReply::finished, this, &Weather::WeatherDataProvider::downloadFinished);
        connect(reply, &QNetworkReply::errorOccurred, this, &Weather::WeatherDataProvider::downloadFinished);
    }

    // Emit "downloading"
    emit downloadingChanged();

    // Handle the case if none of the requests have started (e.g. because
    // no internet is available at all)
    downloadFinished();
}


QDataStream& Weather::operator<<(QDataStream& stream, const WeatherDataProvider::updateLogEntry& ule)
{
    stream << ule.m_time;
    stream << ule.m_bBox;
    return stream;
}


QDataStream& Weather::operator>>(QDataStream& stream, WeatherDataProvider::updateLogEntry& ule)
{
    stream >> ule.m_time;
    stream >> ule.m_bBox;
    return stream;
}
