/***************************************************************************
 *   Copyright (C) 2020-2024 by Stefan Kebekus                             *
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

/* This is a heavily altered version of a file from the metaf library package
 * Copyright (C) 2018-2020 Nick Naumenko (https://gitlab.com/nnaumenko)
 * Distributed under the terms of the MIT license.
 */

#pragma once

#include <QCoreApplication>
#include <QDate>
#include <QObject>

#include <cstring> // Necessary to work around an issue in metaf

#include "navigation/Aircraft.h"
#include "../3rdParty/metaf/include/metaf.hpp"
using namespace metaf;


namespace Weather {


/*! \brief METAR/TAF decoder
 *
 * This class takes METAR or TAF messages in raw form and converts them to
 * human-readable, translated text. This class is not meant to be used directly.
 * Instead, use the classes Weather::METAR or Weather::TAF.
 */

class Decoder : private metaf::Visitor<QString>
{
    Q_DECLARE_TR_FUNCTIONS(Weather::Decoder)

public:
    // This constructor creates an invalid Decoder instance.
    Decoder() = default;

    explicit Decoder(const QString& rawText, const QDate& referenceDate);

    virtual ~Decoder() = default;


    //
    // Methods
    //

    /*! \brief Description of the current weather
     *
     * For METAR messages, this property holds a description of the current
     * weather in translated, human-readable form, such as "low drifting snow"
     * or "light rain". The property can contain an empty string if there is
     * nothing to report.
     *
     * @returns Property currentWeather
     */
    [[nodiscard]] QString currentWeather()
    {
        if (m_currentWeather.isEmpty())
        {
            (void)decodedText({}, {});
        }
        return m_currentWeather;
    }

    /*! \brief Decoded text
     *
     * This method is not thread-safe.
     *
     * @param act Current aircraft, used to determine appropriate units
     *
     * @param time Current time, used to describe points in time
     *
     * @returns Human-readable, translated rich text.
     */
    [[nodiscard]] QString decodedText(const Navigation::Aircraft& act, const QDateTime& time);

    /*! \brief Indicates if raw text could be parsed correctly
     *
     * @returns True if no error
     */
    [[nodiscard]] bool isValid() const
    {
        return (m_parseResult.reportMetadata.error == metaf::ReportError::NONE);
    }


private:
    Q_DISABLE_COPY_MOVE(Decoder)

    // Explanation functions
    static QString explainCloudType(const metaf::CloudType &ct);
    static QString explainDirection(metaf::Direction direction, bool trueCardinalDirections=true);
    static QString explainDirectionSector(const std::vector<metaf::Direction>& dir);
    QString explainDistance(metaf::Distance distance);
    static QString explainDistance_FT(metaf::Distance distance);
    QString explainMetafTime(metaf::MetafTime metafTime);
    static QString explainPrecipitation(metaf::Precipitation precipitation);
    static QString explainPressure(metaf::Pressure pressure);
    static QString explainRunway(metaf::Runway runway);
    QString explainSpeed(metaf::Speed speed);
    static QString explainSurfaceFriction(metaf::SurfaceFriction surfaceFriction);
    static QString explainTemperature(metaf::Temperature temperature);
    static QString explainWaveHeight(metaf::WaveHeight waveHeight);
    QString explainWeatherPhenomena(const metaf::WeatherPhenomena& wp);

    // … toString Methods
    static QString brakingActionToString(metaf::SurfaceFriction::BrakingAction brakingAction);
    static QString cardinalDirectionToString(metaf::Direction::Cardinal cardinal);
    static QString cloudAmountToString(metaf::CloudGroup::Amount amount);
    static QString cloudHighLayerToString(metaf::LowMidHighCloudGroup::HighLayer highLayer);
    static QString cloudLowLayerToString(metaf::LowMidHighCloudGroup::LowLayer lowLayer);
    static QString cloudMidLayerToString(metaf::LowMidHighCloudGroup::MidLayer midLayer);
    static QString cloudTypeToString(metaf::CloudType::Type type);
    static QString convectiveTypeToString(metaf::CloudGroup::ConvectiveType type);
    static QString distanceMilesFractionToString(metaf::Distance::MilesFraction f);
    static QString distanceUnitToString(metaf::Distance::Unit unit);
    static QString layerForecastGroupTypeToString(metaf::LayerForecastGroup::Type type);
    static QString pressureTendencyTrendToString(metaf::PressureTendencyGroup::Trend trend);
    static QString pressureTendencyTypeToString(metaf::PressureTendencyGroup::Type type);
    static QString probabilityToString(metaf::TrendGroup::Probability prob);
    static QString runwayStateDepositsToString(metaf::RunwayStateGroup::Deposits deposits);
    static QString runwayStateExtentToString(metaf::RunwayStateGroup::Extent extent);
    static QString specialWeatherPhenomenaToString(const metaf::WeatherPhenomena& wp);
    static QString stateOfSeaSurfaceToString(metaf::WaveHeight::StateOfSurface stateOfSurface);
    static QString visTrendToString(metaf::VisibilityGroup::Trend trend);
    static QString weatherPhenomenaDescriptorToString(metaf::WeatherPhenomena::Descriptor descriptor);
    static QString weatherPhenomenaQualifierToString(metaf::WeatherPhenomena::Qualifier qualifier);
    static QString weatherPhenomenaWeatherToString(metaf::WeatherPhenomena::Weather weather);

    // visitor Methods
    QString visitCloudGroup(const CloudGroup& group, ReportPart reportPart, const std::string& rawString) override;
    QString visitCloudTypesGroup(const CloudTypesGroup& group, ReportPart reportPart, const std::string& rawString) override;
    QString visitKeywordGroup(const KeywordGroup& group, ReportPart reportPart, const std::string& rawString) override;
    QString visitLayerForecastGroup(const LayerForecastGroup& group, ReportPart reportPart, const std::string& rawString) override;
    QString visitLightningGroup(const LightningGroup& group, ReportPart reportPart, const std::string& rawString) override;
    QString visitLocationGroup(const LocationGroup& group, ReportPart reportPart, const std::string& rawString) override;
    QString visitLowMidHighCloudGroup(const LowMidHighCloudGroup& group, ReportPart reportPart, const std::string& rawString) override;
    QString visitMinMaxTemperatureGroup(const MinMaxTemperatureGroup& group, ReportPart reportPart, const std::string& rawString) override;
    QString visitMiscGroup(const MiscGroup& group,  ReportPart reportPart, const std::string& rawString) override;
    QString visitPressureGroup(const PressureGroup& group, ReportPart reportPart, const std::string& rawString) override;
    QString visitPressureTendencyGroup(const PressureTendencyGroup& group, ReportPart reportPart, const std::string& rawString) override;
    QString visitReportTimeGroup(const ReportTimeGroup& group, ReportPart reportPart, const std::string& rawString) override;
    QString visitPrecipitationGroup(const PrecipitationGroup& group, ReportPart reportPart, const std::string& rawString) override;
    QString visitRunwayStateGroup(const RunwayStateGroup& group, ReportPart reportPart, const std::string& rawString) override;
    QString visitSeaSurfaceGroup(const SeaSurfaceGroup& group, ReportPart reportPart, const std::string& rawString) override;
    QString visitTemperatureGroup(const TemperatureGroup& group, ReportPart reportPart, const std::string& rawString) override;
    QString visitTrendGroup(const TrendGroup& group, ReportPart reportPart, const std::string& rawString) override;
    QString visitUnknownGroup(const UnknownGroup& group, ReportPart reportPart, const std::string& rawString) override;
    QString visitVicinityGroup(const VicinityGroup& group, ReportPart reportPart, const std::string& rawString) override;
    QString visitVisibilityGroup(const VisibilityGroup& group, ReportPart reportPart, const std::string& rawString) override;
    QString visitWeatherGroup(const WeatherGroup& group, ReportPart reportPart, const std::string& rawString) override;
    QString visitWindGroup(const WindGroup& group, ReportPart reportPart, const std::string& rawString) override;


    // Stored for internal use by the method decodedText()
    Navigation::Aircraft m_aircraft;

    // Stored for internal use by the method decodedText()
    QDateTime m_currentTime;

    // Current weather, as read from METAR
    QString m_currentWeather;

    // Reference date
    QDate m_referenceDate;

    // Result of the parser
    ParseResult m_parseResult;
};

} // namespace Weather
