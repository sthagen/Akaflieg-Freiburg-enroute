/***************************************************************************
 *   Copyright (C) 2019-2024 by Stefan Kebekus                             *
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

#include <QImage>
#include <QMimeDatabase>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include "fileFormats/CUP.h"
#include "fileFormats/FPL.h"
#include "fileFormats/MBTILES.h"
#include "fileFormats/MapURL.h"
#include "fileFormats/PLN.h"
#include "fileFormats/TripKit.h"
#include "geomaps/GeoJSON.h"
#include "geomaps/OpenAir.h"
#include "geomaps/VAC.h"
#include "platform/FileExchange_Abstract.h"
#include "traffic/TrafficDataProvider.h"
#include "traffic/TrafficDataSource_File.h"

using namespace Qt::Literals::StringLiterals;


Platform::FileExchange_Abstract::FileExchange_Abstract(QObject *parent)
    : GlobalObject(parent)
{
}



//
// Methods
//

void Platform::FileExchange_Abstract::processFileOpenRequest(const QByteArray& path)
{
    processFileOpenRequest(QString::fromUtf8(path).simplified());
}


void Platform::FileExchange_Abstract::processFileOpenRequest(const QString& path)
{
    /*
     * Check for location MapURLs
     */
    FileFormats::MapURL const mapURL(path);
    if (mapURL.isValid())
    {
        emit openWaypointRequest(mapURL.waypoint());
        return;
    }


    auto file = FileFormats::DataFileAbstract::openFileURL(path);
    const QString myPath = file->fileName();

    /*
     * Get MIME Type
     */

    QMimeDatabase const dataBase;
    auto mimeType = dataBase.mimeTypeForData(file.data());
    qWarning() << path << myPath << mimeType;


    /*
     * Check for various possible file formats/contents
     */

    // Flight Route in GPX format
    if ((mimeType.inherits(QStringLiteral("application/xml"))) || (mimeType.name() == u"application/x-gpx+xml"))
    {
        const FileFormats::PLN pln(path);
        if (pln.isValid())
        {
            emit openFileRequest(path, {}, FlightRoute);
            return;
        }
        const FileFormats::FPL fpl(path);
        if (fpl.isValid())
        {
            emit openFileRequest(path, {}, FlightRoute);
            return;
        }
        emit openFileRequest(path, {}, FlightRouteOrWaypointLibrary);
        return;
    }

    // GeoJSON file
    auto fileContent = GeoMaps::GeoJSON::inspect(myPath);
    if (fileContent == GeoMaps::GeoJSON::flightRoute)
    {
        emit openFileRequest(path, {}, FlightRoute);
        return;
    }
    if (fileContent == GeoMaps::GeoJSON::waypointLibrary)
    {
        emit openFileRequest(path, {}, WaypointLibrary);
        return;
    }
    if (fileContent == GeoMaps::GeoJSON::valid)
    {
        emit openFileRequest(path, {}, FlightRouteOrWaypointLibrary);
        return;
    }

    // FLARM Simulator file
    if (Traffic::TrafficDataSource_File::containsFLARMSimulationData(myPath))
    {
        auto* source = new Traffic::TrafficDataSource_File(false, myPath, GlobalObject::trafficDataProvider());
        GlobalObject::trafficDataProvider()->addDataSource(source); // Will take ownership of source
        source->connectToTrafficReceiver();
        return;
    }

    // MBTiles containing a vector map
    FileFormats::MBTILES mbtiles(myPath);
    if (mbtiles.format() == FileFormats::MBTILES::Vector)
    {
        emit openFileRequest(path, {}, VectorMap);
        return;
    }

    // MBTiles containing a raster map
    if (mbtiles.format() == FileFormats::MBTILES::Raster)
    {
        emit openFileRequest(path, {}, RasterMap);
        return;
    }

    // CUP file
    if (FileFormats::CUP::mimeTypes().contains(mimeType.name()))
    {
        FileFormats::CUP const cup(myPath);
        if (cup.isValid())
        {
            emit openFileRequest(path, {}, WaypointLibrary);
            return;
        }
    }

    // VAC
    if (GeoMaps::VAC::mimeTypes().contains(mimeType.name()))
    {
        GeoMaps::VAC const vac(myPath);
        if (vac.isValid())
        {
            emit openFileRequest(path, vac.name, VAC);
            return;
        }
    }

    // Image
    if (mimeType.name().startsWith(u"image"_s))
    {
        QImage const img(myPath);
        if (!img.isNull())
        {
            emit openFileRequest(path, {}, Image);
            return;
        }
    }

    // TripKits
    if (FileFormats::TripKit::mimeTypes().contains(mimeType.name()))
    {
        FileFormats::TripKit const tripKit(myPath);
        if (tripKit.isValid())
        {
            emit openFileRequest(path, tripKit.name(), TripKit);
            return;
        }
    }

    // ZipFiles
    if (FileFormats::ZipFile::mimeTypes().contains(mimeType.name()))
    {
        FileFormats::ZipFile const zipFile(myPath);
        if (zipFile.isValid())
        {
            emit openFileRequest(path, {}, ZipFile);
            return;
        }
    }

    // OpenAir
    QString info;
    if (GeoMaps::openAir::isValid(myPath, &info))
    {
        emit openFileRequest(path, info, OpenAir);
        return;
    }

    emit openFileRequest(path, {}, UnknownFunction);
}


void Platform::FileExchange_Abstract::processText(const QString& text)
{
    FileFormats::MapURL const mapURL(text);
    if (mapURL.isValid())
    {
        emit openWaypointRequest(mapURL.waypoint());
        return;
    }
    emit unableToProcessText(text);
}
