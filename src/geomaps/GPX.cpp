/***************************************************************************
 *   Copyright (C) 2022-2024 by Stefan Kebekus                             *
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

#include <QFile>

#include "fileFormats/DataFileAbstract.h"
#include "geomaps/GPX.h"

using namespace Qt::Literals::StringLiterals;


//
// Helper functions
//

GeoMaps::Waypoint readWP(QXmlStreamReader& xml) {

    auto tag = xml.name().toString();

    // capture rtept, trkpt or wpt

    QXmlStreamAttributes const attrs = xml.attributes();
    if (!attrs.hasAttribute(QStringLiteral("lon")) || !attrs.hasAttribute(QStringLiteral("lat")))
    {
        return {};
    }

    bool ok = false;
    double const lon = xml.attributes().value(QStringLiteral("lon")).toDouble(&ok);
    if (!ok)
    {
        return {};
    }

    double const lat = xml.attributes().value(QStringLiteral("lat")).toDouble(&ok);
    if (!ok)
    {
        return {};
    }

    QGeoCoordinate pos(lat, lon);

    QString name;
    QString desc;
    QString cmt;
    while (!xml.atEnd() && !xml.hasError())
    {
        if (xml.readNext() == 0U)
        {
            break;
        }

        QString const xmlTag = xml.name().toString();

        if (xml.isStartElement())
        {
            if (xmlTag == u"ele"_s)
            {
                QString const alt_s = xml.readElementText(QXmlStreamReader::SkipChildElements);
                double const alt = alt_s.toFloat(&ok);
                if (!ok)
                {
                    return {};
                }
                pos.setAltitude(alt);
            }
            else if (xmlTag == u"name"_s)
            {
                name = xml.readElementText(QXmlStreamReader::SkipChildElements);
            }
            else if (xmlTag == u"desc"_s)
            {
                desc = xml.readElementText(QXmlStreamReader::SkipChildElements);
            }
            else if (xmlTag == u"cmt"_s)
            {
                cmt = xml.readElementText(QXmlStreamReader::SkipChildElements);
            }

        }
        else if (xml.isEndElement() && (xmlTag == tag))
        {
            break;
        }
    }

    if (name.length() == 0) {
        if (desc.length() != 0) {
            name = desc;
        } else if (cmt.length() != 0) {
            name = cmt;
        }
    }

    // Now create a waypoint, owned by this, and set its name
    GeoMaps::Waypoint wpt;
    wpt = GeoMaps::Waypoint(pos);

    if (!name.isEmpty())
    {
        wpt.setName(name);
    }

    return wpt;
}

//
// Methods
//

bool GeoMaps::GPX::isValid(const QString& fileName)
{
    return !read(fileName).isEmpty();
}

auto GeoMaps::GPX::read(const QString& fileName) -> QVector<GeoMaps::Waypoint>
{
    auto file = FileFormats::DataFileAbstract::openFileURL(fileName);
    if (!file->open(QIODevice::ReadOnly))
    {
        return {};
    }

    QXmlStreamReader xml(file.data());


    // collect all route points and track points and waypoints
    QVector<GeoMaps::Waypoint> rtept;
    QVector<GeoMaps::Waypoint> trkpt;
    QVector<GeoMaps::Waypoint> wpt;

    // lambda function to read a single gpx rtept, trkpt or wpt
    //

    while (!xml.atEnd() && !xml.hasError())
    {
        auto token = xml.readNext();
        if (token == 0U)
        {
            break;
        }

        auto name = xml.name().toString();

        if (xml.isStartElement())
        {
            if (name == u"rtept"_s)
            {
                auto wpt = readWP(xml);
                if (!wpt.isValid())
                {
                    return {};
                }
                rtept << wpt;
            }
            else if (name == u"trkpt"_s)
            {
                auto wpt = readWP(xml);
                if (!wpt.isValid())
                {
                    return {};
                }
                trkpt << wpt;
            }
            else if (name == u"wpt"_s)
            {
                auto wp = readWP(xml);
                if (!wp.isValid())
                {
                    return {};
                }
                wpt << wp;
            }
        }
    }

    // prefer rte over trk over wpt
    // this is a bit arbitrary but seems reasonable to me.
    // Could be made configurable.
    if (!rtept.isEmpty())
    {
        return rtept;
    }
    if (!trkpt.isEmpty())
    {
        return trkpt;
    }
    return wpt;
}
