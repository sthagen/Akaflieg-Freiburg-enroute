/***************************************************************************
 *   Copyright (C) 2021 by Stefan Kebekus                                  *
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

#include <QQmlEngine>

#include "MobileAdaptor.h"
#include "positioning/PositionProvider.h"
#include "traffic/TrafficDataSource_Udp_GDL.h"


// Member functions

Traffic::TrafficDataSource_Udp_GDL::TrafficDataSource_Udp_GDL(quint16 port, QObject *parent) :
    Traffic::TrafficDataSource_Abstract(parent), m_port(port), Crc16Table(256) {

    QQmlEngine::setObjectOwnership(&factor, QQmlEngine::CppOwnership);

    // Initialize CRC16 table
    for(quint16 i = 0; i < 256; i++) {
        quint16 crc = (i << 8U );
        for (int bitctr = 0; bitctr < 8; bitctr++) {
            crc = (crc << 1U) ^ ((crc & 0x8000U) != 0U ? 0x1021U : 0U);
        }
        Crc16Table[i] = crc;
    }

    // Initialize timers
    m_pressureAltitudeTimer.setInterval(5s);
    m_pressureAltitudeTimer.setSingleShot(true);
    m_trueAltitudeTimer.setInterval(5s);
    m_trueAltitudeTimer.setSingleShot(true);

    // Create socket
    socket = new QUdpSocket(this);
    connect(socket, &QUdpSocket::errorOccurred, this, &Traffic::TrafficDataSource_Udp_GDL::onErrorOccurred);
    connect(socket, &QUdpSocket::readyRead, this, &Traffic::TrafficDataSource_Udp_GDL::onReadyRead);
    connect(socket, &QUdpSocket::stateChanged, this, &Traffic::TrafficDataSource_Udp_GDL::onStateChanged);

    // Connect WiFi locker/unlocker
    connect(this, &Traffic::TrafficDataSource_Abstract::receivingHeartbeatChanged, this, &Traffic::TrafficDataSource_Udp_GDL::onReceivingHeartbeatChanged);

    //
    // Initialize properties
    //
    onStateChanged(socket->state());
}


Traffic::TrafficDataSource_Udp_GDL::~TrafficDataSource_Udp_GDL()
{
    Traffic::TrafficDataSource_Udp_GDL::disconnectFromTrafficReceiver();
    setReceivingHeartbeat(false); // This will release the WiFi lock if necessary
}


void Traffic::TrafficDataSource_Udp_GDL::connectToTrafficReceiver()
{
    // Paranoid safety check
    if (socket.isNull()) {
        return;
    }

    socket->abort();
    setErrorString();
    socket->bind(m_port);

    // Update properties
    onStateChanged(socket->state());
}


void Traffic::TrafficDataSource_Udp_GDL::disconnectFromTrafficReceiver()
{
    // Paranoid safety check
    if (socket.isNull()) {
        return;
    }

    // Disconnect socket.
    socket->abort();

    // Update properties
    onStateChanged(socket->state());
}


void Traffic::TrafficDataSource_Udp_GDL::onErrorOccurred(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
    case QAbstractSocket::ConnectionRefusedError:
        setErrorString( tr("The connection was refused by the peer (or timed out).") );
        break;
    case QAbstractSocket::RemoteHostClosedError:
        setErrorString( tr("The remote host closed the connection.") );
        break;
    case QAbstractSocket::HostNotFoundError:
        setErrorString( tr("The host address was not found.") );
        break;
    case QAbstractSocket::SocketAccessError:
        setErrorString( tr("The socket operation failed because the application lacked the required privileges.") );
        break;
    case QAbstractSocket::SocketResourceError:
        setErrorString( tr("The local system ran out of resources.") );
        break;
    case QAbstractSocket::SocketTimeoutError:
        setErrorString( tr("The socket operation timed out.") );
        break;
    case QAbstractSocket::DatagramTooLargeError:
        setErrorString( tr("The datagram was larger than the operating system's limit.") );
        break;
    case QAbstractSocket::NetworkError:
        setErrorString( tr("An error occurred with the network.") );
        break;
    case QAbstractSocket::AddressInUseError:
        setErrorString( tr("The address specified to QAbstractSocket::bind() is already in use and was set to be exclusive.") );
        break;
    case QAbstractSocket::SocketAddressNotAvailableError:
        setErrorString( tr("The address specified to QAbstractSocket::bind() does not belong to the host.") );
        break;
    case QAbstractSocket::UnsupportedSocketOperationError:
        setErrorString( tr("The requested socket operation is not supported by the local operating system.") );
        break;
    case QAbstractSocket::ProxyAuthenticationRequiredError:
        setErrorString( tr("The socket is using a proxy, and the proxy requires authentication.") );
        break;
    case QAbstractSocket::SslHandshakeFailedError:
        setErrorString( tr("The SSL/TLS handshake failed, so the connection was closed.") );
        break;
    case QAbstractSocket::UnfinishedSocketOperationError:
        setErrorString( tr("The last operation attempted has not finished yet (still in progress in the background).") );
        break;
    case QAbstractSocket::ProxyConnectionRefusedError:
        setErrorString( tr("Could not contact the proxy server because the connection to that server was denied.") );
        break;
    case QAbstractSocket::ProxyConnectionClosedError:
        setErrorString( tr("The connection to the proxy server was closed unexpectedly (before the connection to the final peer was established).") );
        break;
    case QAbstractSocket::ProxyConnectionTimeoutError:
        setErrorString( tr("The connection to the proxy server timed out or the proxy server stopped responding in the authentication phase.") );
        break;
    case QAbstractSocket::ProxyNotFoundError:
        setErrorString( tr("The proxy address set with setProxy() (or the application proxy) was not found.") );
        break;
    case QAbstractSocket::ProxyProtocolError:
        setErrorString( tr("The connection negotiation with the proxy server failed, because the response from the proxy server could not be understood.") );
        break;
    case QAbstractSocket::OperationError:
        setErrorString( tr("An operation was attempted while the socket was in a state that did not permit it.") );
        break;
    case QAbstractSocket::SslInternalError:
        setErrorString( tr("The SSL library being used reported an internal error. This is probably the result of a bad installation or misconfiguration of the library.") );
        break;
    case QAbstractSocket::SslInvalidUserDataError:
        setErrorString( tr("Invalid data (certificate, key, cypher, etc.) was provided and its use resulted in an error in the SSL library.") );
        break;
    case QAbstractSocket::TemporaryError:
        setErrorString( tr("A temporary error occurred (e.g., operation would block and socket is non-blocking).") );
        break;
    case QAbstractSocket::UnknownSocketError:
        setErrorString( tr("An unidentified error occurred.") );
        break;
    }

}


void Traffic::TrafficDataSource_Udp_GDL::onReceivingHeartbeatChanged(bool receivingHB)
{
    // Acquire or release WiFi lock as appropriate
    auto* mobileAdaptor = MobileAdaptor::globalInstance();
    if (mobileAdaptor != nullptr) {
        MobileAdaptor::lockWifi(receivingHB);
    }

}


void Traffic::TrafficDataSource_Udp_GDL::onStateChanged(QAbstractSocket::SocketState socketState)
{
    // Paranoid safety check
    if (socket.isNull()) {
        return;
    }

    // Compute new status
    switch( socketState ) {
    case QAbstractSocket::HostLookupState:
        setConnectivityStatus( tr("Performing host name lookup.") );
        break;
    case QAbstractSocket::ConnectingState:
        setConnectivityStatus( tr("Trying to establish a connection.") );
        break;
    case QAbstractSocket::ConnectedState:
        setConnectivityStatus( tr("Connected.") );
        break;
    case QAbstractSocket::BoundState:
        setConnectivityStatus( tr("Bound to an address and port, but not connected yet.") );
        break;
    case QAbstractSocket::ClosingState:
        setConnectivityStatus( tr("Closing.") );
        break;
    default:
        setConnectivityStatus( tr("Not connected.") );
        break;
    }

}


auto pInfoFromOwnshipReport(const QByteArray &decodedData) -> QGeoPositionInfo
{
    // Check message size
    if (decodedData.length() != 27) {
        return {};
    }

    // Find latitude
    auto la0 = static_cast<quint8>(decodedData.at(4));
    auto la1 = static_cast<quint8>(decodedData.at(5));
    auto la2 = static_cast<quint8>(decodedData.at(6));
    qint32 laInt = (la0 << 16) + (la1 << 8) + la2;
    if (laInt > 8388607) {
        laInt -= 16777216;
    }
    double lat = (180.0/0x800000)*laInt;

    // Find longitude
    auto ln0 = static_cast<quint8>(decodedData.at(7));
    auto ln1 = static_cast<quint8>(decodedData.at(8));
    auto ln2 = static_cast<quint8>(decodedData.at(9));
    qint32 lnInt = (ln0 << 16) + (ln1 << 8) + ln2;
    if (lnInt > 8388607) {
        lnInt -= 16777216;
    }
    double lon = (180.0/0x800000)*lnInt;

    // Construct coordinate, generate position info
    QGeoCoordinate coordinate(lat, lon);
    if (!coordinate.isValid()) {
        return {};
    }
    QGeoPositionInfo pInfo(coordinate, QDateTime::currentDateTimeUtc());

    // Find Navigation Accuracy Category for Position
    auto a = static_cast<quint8>(decodedData.at(12)) & 0x0FU;
    switch (a) {
    case 1:
        pInfo.setAttribute(QGeoPositionInfo::HorizontalAccuracy, AviationUnits::Distance::fromNM(10.0).toM() );
        break;
    case 2:
        pInfo.setAttribute(QGeoPositionInfo::HorizontalAccuracy, AviationUnits::Distance::fromNM(4.0).toM() );
        break;
    case 3:
        pInfo.setAttribute(QGeoPositionInfo::HorizontalAccuracy, AviationUnits::Distance::fromNM(2.0).toM() );
        break;
    case 4:
        pInfo.setAttribute(QGeoPositionInfo::HorizontalAccuracy, AviationUnits::Distance::fromNM(1.0).toM() );
        break;
    case 5:
        pInfo.setAttribute(QGeoPositionInfo::HorizontalAccuracy, AviationUnits::Distance::fromNM(0.5).toM() );
        break;
    case 6:
        pInfo.setAttribute(QGeoPositionInfo::HorizontalAccuracy, AviationUnits::Distance::fromNM(0.3).toM() );
        break;
    case 7:
        pInfo.setAttribute(QGeoPositionInfo::HorizontalAccuracy, AviationUnits::Distance::fromNM(0.1).toM() );
        break;
    case 8:
        pInfo.setAttribute(QGeoPositionInfo::HorizontalAccuracy, AviationUnits::Distance::fromNM(0.05).toM() );
        break;
    case 9:
        pInfo.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 30.0 );
        break;
    case 10:
        pInfo.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 10.0 );
        break;
    case 11:
        pInfo.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 3.0 );
        break;
    default:
        break;
    }

    // Find horizontal speed if available
    auto hh0 = static_cast<quint8>(decodedData.at(13));
    auto hh1 = static_cast<quint8>(decodedData.at(14));
    quint32 hhTmp = (hh0 << 4) + (hh1 >> 4);
    if (hhTmp != 0xFFF) {
        AviationUnits::Speed hSpeed = AviationUnits::Speed::fromKN(hhTmp);
        pInfo.setAttribute(QGeoPositionInfo::GroundSpeed, hSpeed.toMPS() );
    }

    // Find vertical speed if available
    auto vv0 = static_cast<quint8>(decodedData.at(14)) & 0x0FU;
    auto vv1 = static_cast<quint8>(decodedData.at(15));
    quint32 vvTmp = (vv0 << 8) + vv1;
    if (vvTmp != 0xFFF) {
        AviationUnits::Speed vSpeed = AviationUnits::Speed::fromFPM(vvTmp*64.0);
        pInfo.setAttribute(QGeoPositionInfo::VerticalSpeed, vSpeed.toMPS() );
    }

    // Find true track if available
    auto mm0 = static_cast<quint8>(decodedData.at(11)) & 0x03U;
    if (mm0 == 1)  {
        auto tt = static_cast<quint8>(decodedData.at(16));
        pInfo.setAttribute(QGeoPositionInfo::Direction, tt*360.0/256.0 );
    }

    return pInfo;
}

#include <QNetworkDatagram>

void Traffic::TrafficDataSource_Udp_GDL::onReadyRead()
{
    //    qWarning() << "Datagram Arrived";
    while (socket->hasPendingDatagrams()) {
        QByteArray data = socket->receiveDatagram().data();
        //        qWarning() << data.toHex();

        //
        // Check if this datagram contains a valid GDL90 Message
        //

        // Trivial consistency checks
        if (!data.startsWith(0x7e)) {
            continue;
        }
        //        qWarning() << "Start correct";
        if (!data.endsWith(0x7e)) {
            continue;
        }
        //        qWarning() << "Ending correct";
        if (data.size() < 5) {
            continue;
        }
        //        qWarning() << "Size roughly correct";


        // Escape character decoding
        QByteArray decodedData;
        decodedData.reserve(data.size());
        bool isEscaped = false;
        for(int i=1; i<data.size()-1; i++) {
            if (data.at(i) == 0x7d) {
                qWarning() << "is escaped";
                isEscaped = true;
                continue;
            }
            if (isEscaped) {
                decodedData.append( static_cast<char>(static_cast<quint8>(data.at(i)) ^ static_cast<quint8>('\x20')) );
                isEscaped = false;
                continue;
            }
            decodedData.append(data.at(i));
        }
        if (isEscaped) {
            continue;
        }
        //        qWarning() << "Escape char decoding correct";
        //        qWarning() << decodedData.toHex();

        // CRC Checksum verification
        quint16 crc = 0;
        foreach(auto byte, decodedData.chopped(2)) {
            crc = Crc16Table.at(crc >> 8U) ^ (crc << 8U) ^ static_cast<quint8>(byte);
        }
        //        qWarning() << "MyCRC     " << QString::number(crc, 16);

        quint16 savedCRC = 0;
        savedCRC += static_cast<quint8>( decodedData.at(decodedData.size()-1) );
        savedCRC = (savedCRC << 8U) + static_cast<quint8>( decodedData.at(decodedData.size()-2) );
        //        qWarning() << "Saved CRC " << QString::number(savedCRC, 16);

        auto messageID = static_cast<quint8>( decodedData.at(0) );

        // Cut off Message ID and checksum
        decodedData.chop(2);
        decodedData = decodedData.mid(1);

        // Heartbeat message
        if (messageID == 0) {
            setReceivingHeartbeat(true);
            return;
        }

        // Ownship report
        if (messageID == 10) {

            // Get position info w/o altitude information
            auto pInfo = pInfoFromOwnshipReport(decodedData);
            if (!pInfo.isValid()) {
                continue;
            }

            // Copy true altitude into pInfo, if known
            if (m_trueAltitudeTimer.isActive()) {
                auto coordinate = pInfo.coordinate();
                coordinate.setAltitude(m_trueAltitude.toM());
                pInfo.setCoordinate(coordinate);
                pInfo.setAttribute(QGeoPositionInfo::VerticalAccuracy, m_trueAltitude_FOM.toM() );
            }

            // Find pressure altitude and update information if need be
            auto dd0 = static_cast<quint8>(decodedData.at(10));
            auto dd1 = static_cast<quint8>(decodedData.at(11));
            quint32 ddTmp = (dd0 << 4) + (dd1 >> 4);
            if (ddTmp != 0xFFF) {
                m_pressureAltitude = AviationUnits::Distance::fromFT(25.0*ddTmp - 1000.0);
                m_pressureAltitudeTimer.start();
            } else {
                m_pressureAltitude = AviationUnits::Distance::fromM( qQNaN() );
            }
            emit pressureAltitudeUpdated(m_pressureAltitude);

            // Update position information and continue
            emit positionUpdated( Positioning::PositionInfo(pInfo) );
            continue;
        }

        // Ownship geometric altitude
        if (messageID == 11) {
            // Find geometric alt
            auto dd0 = static_cast<quint8>(decodedData.at(0));
            auto dd1 = static_cast<quint8>(decodedData.at(1));
            qint32 ddInt = (dd0 << 8) + dd1;
            if (ddInt > 32767) {
                ddInt -= 65536;
            }           

            // Find geometric figure of merit
            auto vm0 = static_cast<quint8>(decodedData.at(2)) & 0x7FU;
            auto vm1 = static_cast<quint8>(decodedData.at(3));
            auto vmInt = (vm0 << 8) + vm1;

            m_trueAltitude = AviationUnits::Distance::fromFT(ddInt*5.0);
            m_trueAltitude_FOM = AviationUnits::Distance::fromM(vmInt);
            m_trueAltitudeTimer.start();
        }

        // Traffic report
        if (messageID == 20) {

            // Get position info w/o altitude information
            auto pInfo = pInfoFromOwnshipReport(decodedData);
            if (!pInfo.isValid()) {
                continue;
            }

            // Get ID
            auto id0 = static_cast<quint8>(decodedData.at(0)) & 0x0FU;
            auto id1 = static_cast<quint8>(decodedData.at(1));
            auto id2 = static_cast<quint8>(decodedData.at(2));
            auto id3 = static_cast<quint8>(decodedData.at(3));
            auto id = QString::number(id0, 16) + QString::number(id1, 16) + QString::number(id2, 16) + QString::number(id3, 16);

            // Alert
            auto s0 = static_cast<quint8>(decodedData.at(0)) >> 4;
            auto alert = (s0 == 1) ? 1 : 0;
            qWarning() << "id" << id << s0;

            // Traffic type
            auto ee = static_cast<quint8>(decodedData.at(17));
            auto type = Traffic::TrafficFactor::unknown;
            switch(ee) {
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
                type = Traffic::TrafficFactor::Aircraft;
                break;
            case 6:
                type = Traffic::TrafficFactor::Jet;
                break;
            case 7:
                type = Traffic::TrafficFactor::Copter;
                break;
            case 9:
                type = Traffic::TrafficFactor::Glider;
                break;
            case 10:
                type = Traffic::TrafficFactor::Balloon;
                break;
            case 11:
                type = Traffic::TrafficFactor::Skydiver;
                break;
            case 14:
                type = Traffic::TrafficFactor::Drone;
                break;
            case 19:
                type = Traffic::TrafficFactor::StaticObstacle;
                break;
            default:
                break;
            }

            // Compute true altitude and altitude distance of traffic if
            // a recent pressure altitude reading for owncraft exists.
            AviationUnits::Distance vDist {};
            if (m_pressureAltitudeTimer.isActive()) {
                auto dd0 = static_cast<quint8>(decodedData.at(10));
                auto dd1 = static_cast<quint8>(decodedData.at(11));
                quint32 ddTmp = (dd0 << 4) + (dd1 >> 4);
                if (ddTmp != 0xFFF) {
                    auto trafficPressureAltitude = AviationUnits::Distance::fromFT(25.0*ddTmp - 1000.0);
                    vDist = trafficPressureAltitude - m_pressureAltitude;

                    // Compute true altitude of traffic if possible
                    if (m_trueAltitudeTimer.isActive()) {
                        auto trafficTrueAltitude = m_trueAltitude + vDist;
                        auto coordinate = pInfo.coordinate();
                        coordinate.setAltitude(trafficTrueAltitude.toM());
                        pInfo.setCoordinate(coordinate);
                    }
                }
            }

            // Compute horizontal distance to traffic if our own position
            // is known.
            AviationUnits::Distance hDist {};
            auto* positionProviderPtr = Positioning::PositionProvider::globalInstance();
            if (positionProviderPtr != nullptr) {
                auto ownShipCoordinate = positionProviderPtr->positionInfo().coordinate();
                auto trafficCorrdinate = pInfo.coordinate();
                if (ownShipCoordinate.isValid() && trafficCorrdinate.isValid()) {
                    hDist = AviationUnits::Distance::fromM( ownShipCoordinate.distanceTo(trafficCorrdinate) );
                }
            }

            // Callsign of traffic
            auto callSignBytes = QString::fromLatin1(decodedData.mid(18,8)).simplified();
            qWarning() << "Traffic callsign" << callSignBytes;


            factor.setData(alert, id, hDist, vDist, type, pInfo);
            emit factorWithPosition(factor);


/*

            // Find pressure altitude
            auto dd0 = static_cast<quint8>(decodedData.at(10));
            auto dd1 = static_cast<quint8>(decodedData.at(11));
            quint32 ddTmp = (dd0 << 4) + (dd1 >> 4);
            if (ddTmp != 0xFFF) {
                AviationUnits::Distance pAlt = AviationUnits::Distance::fromFT(25.0*ddTmp - 1000.0);
//                emit pressureAltitudeUpdated(pAlt);
            }


            // Update position information and return
//            emit positionUpdated( Positioning::PositionInfo(pInfo) );
            return;
            */
        }


        switch(messageID) {
        case 7:
            qWarning() << "UAT Uplink";
            break;
        case 0x65:
            qWarning() << "ID Message or AHRS Message";
            break;
        default:
            qWarning() << "Unknown" << static_cast<quint8>(decodedData.at(0));

        }
    }

}


/*
#include "SimInterface.h"

#include <QNetworkDatagram>
#include <QByteArray>
#include <QNetworkInterface>

SimInterface::SimInterface(QObject *parent) : QObject(parent)
{
}

SimInterface::~SimInterface()
{
    delete _udpSocket;
}

bool SimInterface::start()
{
    if (!_udpSocket) {
        _udpSocket = new QUdpSocket(this);
    }

    bool bBindSuccess = _udpSocket->bind(49002);
    connect(_udpSocket, &QUdpSocket::readyRead, this, &SimInterface::readPendingDatagrams);

    _timeoutPosUpdate.setSingleShot(true);
    connect(&_timeoutPosUpdate, &QTimer::timeout, this, &SimInterface::timeout);

    return bBindSuccess;
}

void SimInterface::stop()
{
    _timeoutPosUpdate.stop();
    _udpSocket->close();
}

QGeoPositionInfo SimInterface::lastKnownPosition() const
{
    return _geoPos;
}

void SimInterface::readPendingDatagrams()
{
    while (_udpSocket->hasPendingDatagrams())
    {
        QString str = _udpSocket->receiveDatagram().data();
        QStringList list = str.split(QLatin1Char(','));

        if (list[0].contains("XGPS")) {
            if (_simName != list[0].remove(0,4)) {
                if ("1" == list[0]) {
                    if (_simName != "X-Plane") {
                        _simName = "X-Plane";
                    }
                }
                else {
                    _simName = list[0];
                }
            }

            _geoPos.setCoordinate(QGeoCoordinate(list[2].toDouble(),
                                                 list[1].toDouble(),
                                                 list[3].toDouble()));
            _geoPos.setAttribute(QGeoPositionInfo::Direction, list[4].toDouble());
            _geoPos.setAttribute(QGeoPositionInfo::GroundSpeed, list[5].toDouble());

            _geoPos.setTimestamp(QDateTime::currentDateTimeUtc());

            _timeoutPosUpdate.start(_timeoutThreshold);
            emit positionUpdated(_geoPos);
        }
        else if (list[0].contains("XTRA")) {
            QString targetID = list[1];
            QGeoPositionInfo geoPositionInfo(QGeoCoordinate(list[2].toDouble(),
                                                            list[3].toDouble(),
                                                            list[4].toDouble()*0.3048),
                                             QDateTime::currentDateTimeUtc());

            geoPositionInfo.setAttribute(QGeoPositionInfo::VerticalSpeed, list[5].toDouble()/0.3048/60.0);
            geoPositionInfo.setAttribute(QGeoPositionInfo::Direction, list[7].toDouble());
            geoPositionInfo.setAttribute(QGeoPositionInfo::GroundSpeed, list[8].toDouble()*1.852/3.6);

            auto traffic = Navigation::Traffic();

            traffic.setData(0,
                            targetID,
                            AviationUnits::Distance::fromM(_geoPos.coordinate().distanceTo(geoPositionInfo.coordinate())),
                            AviationUnits::Distance::fromM(geoPositionInfo.coordinate().altitude() - _geoPos.coordinate().altitude()),
                            AviationUnits::Speed::fromMPS(_geoPos.attribute(QGeoPositionInfo::VerticalSpeed)),
                            AviationUnits::Speed(),
                            Navigation::Traffic::unknown,
                            geoPositionInfo );

            emit trafficUpdated(traffic);
        }
    }
}
*/