/***************************************************************************
 *   Copyright (C) 2019-2023 by Stefan Kebekus                             *
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

#pragma once

#include <QGeoPositionInfo>
#include <QQmlEngine>
//#include <chrono>

#include "units/Angle.h"
#include "units/Distance.h"
#include "units/Speed.h"

using namespace std::chrono_literals;


namespace Positioning {

/*! \brief Geographic position
 *
 *  This class is a thin wrapper around QGeoPositionInfo. It exports the data
 *  from QGeoPositionInfo in a way that can be read from QML. In addition to
 *  QGeoPositionInfo, the class also contains information about the pressure
 *  altitude.
 */

class PositionInfo
{
    Q_GADGET
    QML_VALUE_TYPE(positionInfo)

public:
    /*! \brief Default Constructor */
    PositionInfo() = default;

    /*! \brief Constructor
     *
     * @param info QGeoPositionInfo this is copied into this class
     *
     * @param source Name of the source that generated with instance
     */
    explicit PositionInfo(const QGeoPositionInfo& info, const QString& source);

    /*! \brief Coordinate
     *
     *  If the coordinate contains altitude information, this refers to
     *  true altitude.
     *
     *  @returns Coordinate
     */
    [[nodiscard]] Q_INVOKABLE QGeoCoordinate coordinate() const
    {
        return m_positionInfo.coordinate();
    }

    /*! \brief Ground speed
     *
     *  @returns Ground speed or NaN if unknown.
     */
    [[nodiscard]] Q_INVOKABLE Units::Speed groundSpeed() const;

    /*! \brief Validity
     *
     *  @returns True if the underlying QGeoPositionInfo is valid and if
     *  its age is less then PositionInfo::lifetime.
     */
    [[nodiscard]] Q_INVOKABLE bool isValid() const;

    /*! \brief Position error estimate
     *
     *  @returns Position error estimate or NaN if unknown.
     */
    [[nodiscard]] Q_INVOKABLE Units::Distance positionErrorEstimate() const;

    /*! \brief Name of source, as set in the constructor
     *
     *  @returns Name of source
     */
    [[nodiscard]] Q_INVOKABLE QString source() const {return m_source;}

    /*! \brief Elevation of terrain at a given coordinate, above sea level
     *
     *  @returns Elevation of the terrain at position, or
     *  NaN if the terrain elevation is unknown
     */
    [[nodiscard]] Q_INVOKABLE Units::Distance terrainElevationAMSL();

    /*! \brief Timestamp
     *
     *  @returns Timestamp of the position info.
     */
    [[nodiscard]] Q_INVOKABLE QDateTime timestamp() const
    {
        return m_positionInfo.timestamp().toUTC();
    }

    /*! \brief Timestamp string
     *
     *  @returns Timestamp of the position info, as a string.
     */
    [[nodiscard]] Q_INVOKABLE QString timestampString() const
    {
        return m_positionInfo.timestamp().toUTC().time().toString(QStringLiteral("HH:mm:ss"))+ " UTC";
    }

    /*! \brief True Altitude above ground level
     *
     *  @returns True altitude with geoid correction taken into account or NaN
     *  if unknown.
     */
    [[nodiscard]] Q_INVOKABLE Units::Distance trueAltitudeAGL();

    /*! \brief True Altitude above main sea level
     *
     *  @returns True altitude with geoid correction taken into account or NaN
     *  if unknown.
     */
    [[nodiscard]] Q_INVOKABLE Units::Distance trueAltitudeAMSL() const;

    /*! \brief True altitude error estimate
     *
     *  @returns True altitude error estimate or NaN if unknown.
     */
    [[nodiscard]] Q_INVOKABLE Units::Distance trueAltitudeErrorEstimate() const;

    /*! \brief True track
     *
     *  @returns True track or NaN if unknown.
     */
    [[nodiscard]] Q_INVOKABLE Units::Angle trueTrack() const;

    /*! \brief True track error estimate
     *
     *  @returns True track error estimate or NaN if unknown.
     */
    [[nodiscard]] Q_INVOKABLE Units::Angle trueTrackErrorEstimate() const;

    /*! \brief Magnetic variation
     *
     *  @returns Magnetic variation or NaN if unknown.
     */
    [[nodiscard]] Q_INVOKABLE Units::Angle variation() const;

    /*! \brief Vertical speed
     *
     *  @returns Vertical speed or NaN if unknown.
     */
    [[nodiscard]] Q_INVOKABLE Units::Speed verticalSpeed() const;

    /*! \brief Comparison: equal
     *
     *  @param rhs Right hand side of the comparison
     *
     *  @returns Result of the comparison
     */
    Q_INVOKABLE bool operator==(const Positioning::PositionInfo &rhs) const
    {
        return (m_positionInfo == rhs.m_positionInfo);
    }

    /*! \brief Conversion */
    operator QGeoPositionInfo() const
    {
        return m_positionInfo;
    }

    /*! \brief Liftetime of geographic positioning information
     *
     * Geographic position information is considered valid only for
     * this amount of time after it has been received.
     */
#if defined(Q_OS_ANDROID) or defined(Q_OS_IOS)
    static constexpr auto lifetime = 20s;
#else
    static constexpr auto lifetime = 20min;
#endif


private:
    QGeoPositionInfo m_positionInfo;
    QString m_source;
    Units::Distance m_terrainAMSL;
    Units::Distance m_trueAltitudeAGL;
};

} // namespace Positioning

Q_DECLARE_METATYPE(Positioning::PositionInfo)
