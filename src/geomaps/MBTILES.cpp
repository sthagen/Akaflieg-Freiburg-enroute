/***************************************************************************
 *   Copyright (C) 2022 by Stefan Kebekus                                  *
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

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>

#include "geomaps/MBTILES.h"

GeoMaps::MBTILES::Format GeoMaps::MBTILES::format(const QString& fileName)
{
    GeoMaps::MBTILES::Format result = Unknown;

    { // Parenthesis necessary, because testDB needs to be deconstructed before QSqlDatabase::removeDatabase is called
        auto testDB = QSqlDatabase::addDatabase("QSQLITE", "import test: "+fileName);
        testDB.setDatabaseName(fileName);

        if (testDB.open()) {
            QSqlQuery query(testDB);
            if (query.exec("select name, value from metadata where name='format';")) {
                if (query.first()) {
                    auto format = query.value(1).toString();
                    if (format == "pbf") {
                        result = Vector;
                    }
                    if ((format == "jpg") || (format == "png") || (format == "webp")) {
                        result = Raster;
                    }
                }
            }
            testDB.close();
        }
    }
    QSqlDatabase::removeDatabase("import test: "+fileName);

    return result;

}
