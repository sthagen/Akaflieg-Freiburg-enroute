/***************************************************************************
 *   Copyright (C) 2019-2020 by Stefan Kebekus                             *
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

#include <QSettings>
#include <QVariant>
#include <QtMath>

#include "AviationUnits.h"
#include "positioning/PositionProvider.h"
#include "traffic/TrafficDataProvider.h"


// Static instance of this class. Do not analyze, because of many unwanted warnings.
#ifndef __clang_analyzer__
Q_GLOBAL_STATIC(Positioning::PositionProvider, PositionProviderStatic);
#endif


Positioning::PositionProvider::PositionProvider(QObject *parent)
    : QObject(parent)
{
    source = QGeoPositionInfoSource::createDefaultSource(this);

    if (source != nullptr) {
        sourceStatus = source->error();
        connect(source, SIGNAL(error(QGeoPositionInfoSource::Error)), this, SLOT(error(QGeoPositionInfoSource::Error)));
        connect(source, &QGeoPositionInfoSource::updateTimeout, this, &PositionProvider::timeout);
        connect(source, &QGeoPositionInfoSource::positionUpdated, this, &PositionProvider::onPositionUpdated_Sat);
    }

    auto* trafficDataProvider = Traffic::TrafficDataProvider::globalInstance();
    if (trafficDataProvider != nullptr) {
        connect(trafficDataProvider, &Traffic::TrafficDataProvider::positionInfoChanged, this, &PositionProvider::onPositionUpdated_TrafficDataProvider);
        connect(trafficDataProvider, &Traffic::TrafficDataProvider::barometricAltitudeChanged, this, &PositionProvider::barometricAltitudeChanged);
    }

    QSettings settings;
    QGeoCoordinate tmp;
    tmp.setLatitude(settings.value(QStringLiteral("PositionProvider/lastValidLatitude"), _lastValidCoordinate.latitude()).toDouble());
    tmp.setLongitude(settings.value(QStringLiteral("PositionProvider/lastValidLongitude"), _lastValidCoordinate.longitude()).toDouble());
    tmp.setAltitude(settings.value(QStringLiteral("PositionProvider/lastValidAltitude"), _lastValidCoordinate.altitude()).toDouble());
    if ((tmp.type() == QGeoCoordinate::Coordinate2D) || (tmp.type() == QGeoCoordinate::Coordinate3D)) {
        _lastValidCoordinate = tmp;
    }
    _lastValidTrack = qBound(0, settings.value(QStringLiteral("PositionProvider/lastValidTrack"), 0).toInt(), 359);

    if (source != nullptr) {
        source->startUpdates();
        if ((source->supportedPositioningMethods() & QGeoPositionInfoSource::SatellitePositioningMethods) == QGeoPositionInfoSource::SatellitePositioningMethods) {
            _geoid = new Positioning::Geoid;
        }
    }

    // Adjust and connect timeoutCounter
    timeoutCounter.setSingleShot(true);
    connect(&timeoutCounter, &QTimer::timeout, this, &PositionProvider::timeout);
}


Positioning::PositionProvider::~PositionProvider()
{
    QSettings settings;

    settings.setValue(QStringLiteral("PositionProvider/lastValidLatitude"), _lastValidCoordinate.latitude());
    settings.setValue(QStringLiteral("PositionProvider/lastValidLongitude"), _lastValidCoordinate.longitude());
    settings.setValue(QStringLiteral("PositionProvider/lastValidAltitude"), _lastValidCoordinate.altitude());
    settings.setValue(QStringLiteral("PositionProvider/lastValidTrack"), _lastValidTrack);
    delete source;
    delete _geoid;
}


auto Positioning::PositionProvider::altitudeInFeet() const -> int
{
    if (lastInfo.coordinate().type() != QGeoCoordinate::Coordinate3D) {
        return 0;
    }

    auto alt = AviationUnits::Distance::fromM(lastInfo.coordinate().altitude());
    return qRound(alt.toFeet());
}


auto Positioning::PositionProvider::altitudeInFeetAsString() const -> QString
{
    if (lastInfo.coordinate().type() != QGeoCoordinate::Coordinate3D) {
        return QStringLiteral("-");
    }

    return myLocale.toString(altitudeInFeet()) + " ft";
}


void Positioning::PositionProvider::error(QGeoPositionInfoSource::Error newSourceStatus)
{
    // Save old status and set sourceStatus to QGeoPositionInfoSource::NoError
    auto oldStatus = status();
    sourceStatus = newSourceStatus;

    // If there really is an error, reset lastInfo and cancel all counters
    if (newSourceStatus != QGeoPositionInfoSource::NoError) {
        lastInfo = QGeoPositionInfo();
        timeoutCounter.stop();
    }

    if (oldStatus != status()) {
        emit iconChanged();
        emit statusChanged();
        emit update();
    }
}


auto Positioning::PositionProvider::globalInstance() -> PositionProvider *
{
#ifndef __clang_analyzer__
    return PositionProviderStatic;
#else
    return nullptr;
#endif
}


auto Positioning::PositionProvider::groundSpeedInKnots() const -> int
{
    auto gsInMPS = groundSpeedInMetersPerSecond();

    if (gsInMPS < 0.0) {
        return -1;
    }

    auto groundSpeed = AviationUnits::Speed::fromMPS(gsInMPS);
    return qRound(groundSpeed.toKT());
}


auto Positioning::PositionProvider::groundSpeedInKnotsAsString() const -> QString
{
    auto gsInKnots = groundSpeedInKnots();

    if (gsInKnots < 0) {
        return QStringLiteral("-");
    }
    return myLocale.toString(gsInKnots) + " kt";
}


auto Positioning::PositionProvider::groundSpeedInKMHAsString() const -> QString
{
    auto gsInMPS = groundSpeedInMetersPerSecond();

    if (gsInMPS < 0.0) {
        return QStringLiteral("-");
    }

    auto gsInKMH = qRound(AviationUnits::Speed::fromMPS(gsInMPS).toKMH());
    return myLocale.toString(gsInKMH) + " km/h";
}


auto Positioning::PositionProvider::groundSpeedInMetersPerSecond() const -> qreal
{
    if (!lastInfo.isValid()) {
        return -1.0;
    }
    if (!lastInfo.hasAttribute(QGeoPositionInfo::GroundSpeed)) {
        return -1.0;
    }

    auto groundSpeed = lastInfo.attribute(QGeoPositionInfo::GroundSpeed);
    if (!qIsFinite(groundSpeed)) {
        return -1.0;
    }
    if (groundSpeed < 0.0) {
        return -1.0;
    }

    return groundSpeed;
}


auto Positioning::PositionProvider::horizontalPrecisionInMeters() const -> int
{
    if (!lastInfo.isValid()) {
        return -1;
    }
    if (!lastInfo.hasAttribute(QGeoPositionInfo::HorizontalAccuracy)) {
        return -1;
    }

    auto horizontalPrecision = lastInfo.attribute(QGeoPositionInfo::HorizontalAccuracy);
    if (!qIsFinite(horizontalPrecision)) {
        return -1;
    }
    if (horizontalPrecision <= 0.0) {
        return -1;
    }

    return static_cast<int>(qFloor(horizontalPrecision+0.5));
}


auto Positioning::PositionProvider::horizontalPrecisionInMetersAsString() const -> QString
{
    auto _horizontalPrecisionInMeters = horizontalPrecisionInMeters();

    if (_horizontalPrecisionInMeters < 0) {
        return QStringLiteral("-");
    }
    return QStringLiteral("±%1 m").arg(_horizontalPrecisionInMeters);
}


auto Positioning::PositionProvider::icon() const -> QString
{
    if (status() != OK) {
        return QStringLiteral("/icons/self-noPosition.svg");
    }

    if (track() < 0) {
        return QStringLiteral("/icons/self-noDirection.svg");
    }

    return QStringLiteral("/icons/self-withDirection.svg");
}


auto Positioning::PositionProvider::lastValidCoordinate() const -> QGeoCoordinate
{
    return _lastValidCoordinate;
}


auto Positioning::PositionProvider::lastValidCoordinateStatic() -> QGeoCoordinate
{
    // Find out that unit system we should use
    auto *PositionProvider = PositionProvider::globalInstance();
    if (PositionProvider != nullptr) {
        return PositionProvider->lastValidCoordinate();
    }
    // Fallback in the very unlikely case that no global object exists
    return QGeoCoordinate();
}


auto Positioning::PositionProvider::latitudeAsString() const -> QString
{
    if (!lastInfo.isValid()) {
        return QStringLiteral("-");
    }

    auto x = lastInfo.coordinate().latitude();
    if ((!qIsFinite(x)) || (x < -90.0) || (x > 90.0)) {
        return QStringLiteral("-");
    }

    auto angle = AviationUnits::Angle::fromDEG(x);
    QString latString = angle.toString();
    latString += (x >= 0) ? QStringLiteral(" N") : QStringLiteral(" S");
    return latString;
}


auto Positioning::PositionProvider::longitudeAsString() const -> QString
{
    if (!lastInfo.isValid()) {
        return QStringLiteral("-");
    }

    auto x = lastInfo.coordinate().longitude();
    if ((!qIsFinite(x)) || (x < -180.0) || (x > 180.0)) {
        return QStringLiteral("-");
    }

    auto angle = AviationUnits::Angle::fromDEG(x);
    QString lonString = angle.toString();

    lonString += (x >= 0) ? QStringLiteral(" E") : QStringLiteral(" W");
    return lonString;
}


void Positioning::PositionProvider::onPositionUpdated_Sat(const QGeoPositionInfo &info)
{
    auto* trafficDataProvider = Traffic::TrafficDataProvider::globalInstance();
    if (trafficDataProvider != nullptr) {
        if (trafficDataProvider->positionInfo().isValid()) {
            return;
        }
    }


    auto correctedInfo = info;
    if ((_geoid != nullptr) && (info.coordinate().type() == QGeoCoordinate::Coordinate3D)) {
        auto geoidCorrection = _geoid->operator()(static_cast<qreal>(info.coordinate().latitude()), static_cast<qreal>(info.coordinate().longitude()));
        correctedInfo.setCoordinate( correctedInfo.coordinate().atDistanceAndAzimuth(0, 0, -geoidCorrection) );
    }

    statusUpdate(info);
}


void Positioning::PositionProvider::onPositionUpdated_TrafficDataProvider()
{
    auto* trafficDataProvider = Traffic::TrafficDataProvider::globalInstance();
    if (trafficDataProvider == nullptr) {
        return;
    }

    statusUpdate(trafficDataProvider->positionInfo());
}


auto Positioning::PositionProvider::status() const -> PositionProvider::Status
{
    if (source == nullptr) {
        return Status::Error;
    }

    if (sourceStatus != QGeoPositionInfoSource::NoError) {
        return Status::Error;
    }

    if (!timeoutCounter.isActive()) {
        return Status::Timeout;
    }

    return Status::OK;
}


auto Positioning::PositionProvider::statusAsString() const -> QString
{
    if (source == nullptr) {
        return tr("Not installed or access denied");
    }

    if (sourceStatus == QGeoPositionInfoSource::AccessError) {
        return tr("Access denied");
    }

    if (sourceStatus == QGeoPositionInfoSource::ClosedError) {
        return tr("Connection to satellite system lost");
    }

    if (sourceStatus != QGeoPositionInfoSource::NoError) {
        return tr("Unknown error");
    }

    if (!timeoutCounter.isActive()) {
        return tr("Waiting for signal");
    }

    return tr("OK");
}


void Positioning::PositionProvider::statusUpdate(const QGeoPositionInfo &info)
{
    // Ignore invalid infos
    if (!info.isValid()) {
        return;
    }

    // Save old values and set sourceStatus to QGeoPositionInfoSource::NoError
    auto oldStatus = status();
    auto oldIcon   = icon();
    sourceStatus = QGeoPositionInfoSource::NoError;

    lastInfo = info;
    if ((_geoid != nullptr) && (lastInfo.coordinate().type() == QGeoCoordinate::Coordinate3D)) {
        auto geoidCorrection = _geoid->operator()(static_cast<qreal>(info.coordinate().latitude()), static_cast<qreal>(info.coordinate().longitude()));
        lastInfo.setCoordinate( lastInfo.coordinate().atDistanceAndAzimuth(0, 0, -geoidCorrection) );
    }
    timeoutCounter.start(timeoutThreshold);

    // Inform others
    if (oldStatus != Status::OK) {
        emit statusChanged();
    }

    _lastValidCoordinate = info.coordinate();
    emit update();

    // Change _isInFlight if appropriate.
    if (_isInFlight) {
        // If we are in flight at present, go back to ground mode only if the ground speed is less than minFlightSpeedInKT-flightSpeedHysteresis
        if (groundSpeedInKnots() < minFlightSpeedInKT-flightSpeedHysteresis) {
            _isInFlight = false;
            emit isInFlightChanged();
        }
    } else {
        // If we are on the ground at present, go to flight mode only if the ground sped is more than minFlightSpeedInKT
        if (groundSpeedInKnots() > minFlightSpeedInKT) {
            _isInFlight = true;
            emit isInFlightChanged();
        }
    }

    // emit iconChanged() if appropriate
    if (oldIcon != icon()) {
        emit iconChanged();
    }

    // emit lastValidTrackChanged if appropriate
    auto newTrack = track();
    if ((newTrack >= 0) && (newTrack != _lastValidTrack)) {
        _lastValidTrack = newTrack;
        emit lastValidTrackChanged();
    }
}


void Positioning::PositionProvider::timeout()
{
    // Clear lastInfo, stop counter
    lastInfo = QGeoPositionInfo();

    emit iconChanged();
    emit statusChanged();
    emit update();
}


auto Positioning::PositionProvider::timestampAsString() const -> QString
{
    if (!lastInfo.isValid()) {
        return QStringLiteral("-");
    }
    QDateTime timeStamp = lastInfo.timestamp();
    if (!timeStamp.isValid()) {
        return QStringLiteral("-");
    }

    return timeStamp.time().toString(QStringLiteral("HH:mm:ss")) + " UTC";
}


auto Positioning::PositionProvider::track() const -> int
{
    if (groundSpeedInKnots() < 4) {
        return -1;
    }
    if (!lastInfo.hasAttribute(QGeoPositionInfo::Direction)) {
        return -1;
    }

    auto track = lastInfo.attribute(QGeoPositionInfo::Direction);
    if (!qIsFinite(track)) {
        return -1;
    }

    auto intTrack = static_cast<int>(qFloor(track+0.5));
    if ((intTrack < 0) || (intTrack > 360)) {
        return -1;
    }

    return intTrack % 360;
}


auto Positioning::PositionProvider::trackAsString() const -> QString
{
    auto _track = track();

    if (_track < 0) {
        return QStringLiteral("-");
    }
    return QStringLiteral("%1°").arg(_track);
}


auto Positioning::PositionProvider::wayTo(const QGeoCoordinate& position) const -> QString
{
    // Paranoid safety checks
    if (status() != PositionProvider::OK) {
        return QString();
    }
    if (!position.isValid()) {
        return QString();
    }
    if (!_lastValidCoordinate.isValid()) {
        return QString();
    }

    auto dist = AviationUnits::Distance::fromM(_lastValidCoordinate.distanceTo(position));
    auto QUJ = qRound(_lastValidCoordinate.azimuthTo(position));

    if (GlobalSettings::useMetricUnitsStatic()) {
        return QStringLiteral("DIST %1 km • QUJ %2°").arg(dist.toKM(), 0, 'f', 1).arg(QUJ);
    }
    return QStringLiteral("DIST %1 NM • QUJ %2°").arg(dist.toNM(), 0, 'f', 1).arg(QUJ);
}


auto Positioning::PositionProvider::barometricAltitude()  -> AviationUnits::Distance
{
    auto* trafficDataProvider = Traffic::TrafficDataProvider::globalInstance();
    if (trafficDataProvider == nullptr) {
        return AviationUnits::Distance();
    }

    return trafficDataProvider->barometricAltitude();
}


auto Positioning::PositionProvider::flightLevel() -> QString
{
    auto baroAlt = barometricAltitude();
    if (!baroAlt.isFinite()) {
        return "-";
    }

    return QString("FL%1").arg(qRound(baroAlt.toFeet() / 100.0), 3, 10, QChar('0') );
}