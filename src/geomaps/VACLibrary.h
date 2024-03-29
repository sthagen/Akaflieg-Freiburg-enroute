/***************************************************************************
 *   Copyright (C) 2024 by Stefan Kebekus                                  *
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

#include <QStandardPaths>

#include "geomaps/VAC.h"


namespace GeoMaps
{

#warning docu

class VACLibrary : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    /*! \brief Constructor */
    VACLibrary();

    /*! \brief Destructor */
    ~VACLibrary() override = default;

    //
    // Properties
    //

#warning docu
    Q_PROPERTY(bool isEmpty READ isEmpty NOTIFY dataChanged)

#warning docu
    Q_PROPERTY(QVector<GeoMaps::VAC> vacs READ vacs NOTIFY dataChanged)


    //
    // Getter Methods
    //

#warning docu
    [[nodiscard]] bool isEmpty() const { return m_vacs.isEmpty(); }

#warning docu, want to sort alphabetically
    [[nodiscard]] QVector<GeoMaps::VAC> vacs() const { return m_vacs; }


    //
    // Setter Methods
    //



    //
    // Methods
    //

    Q_INVOKABLE void clear();

    [[nodiscard]] Q_INVOKABLE QVector<GeoMaps::VAC> vacsByDistance(const QGeoCoordinate& position);

    [[nodiscard]] Q_INVOKABLE QString importVAC(const QString& fileName, QString newName);

    [[nodiscard]] Q_INVOKABLE QString importTripKit(const QString& fileName);

    Q_INVOKABLE void deleteVAC(const QString& baseName);

    [[nodiscard]] Q_INVOKABLE QString renameVAC(const QString& oldBaseName, const QString& newBaseName);

    [[nodiscard]] Q_INVOKABLE GeoMaps::VAC get(const QString& name);

    [[nodiscard]] Q_INVOKABLE void save();

signals:
    /*! \brief Notifier signal */
    void dataChanged();

    /*! \brief Progress report when importing a trip kit
     *
     *  @param percent A number between 0.0 and 1.0.
     */
    void importTripKitStatus(double percent);

private:
    Q_DISABLE_COPY_MOVE(VACLibrary)

    QVector<GeoMaps::VAC> m_vacs;
    QString m_vacDirectory {QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/VAC"};

};

} // namespace GeoMaps

