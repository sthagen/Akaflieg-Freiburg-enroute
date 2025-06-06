/***************************************************************************
 *   Copyright (C) 2019-2022 by Stefan Kebekus                             *
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

#include <QDBusInterface>

#include "PlatformAdaptor_Abstract.h"

namespace Platform {

/*! \brief Implementation of PlatformAdaptor for Linux desktop devices */

class PlatformAdaptor : public PlatformAdaptor_Abstract
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    /*! \brief Standard constructor
     *
     * @param parent Standard QObject parent pointer
     */
    explicit PlatformAdaptor(QObject* parent = nullptr);

    ~PlatformAdaptor() override = default;


    //
    // Methods
    //

    /*! \brief Implements pure virtual method from PlatformAdaptor_Abstract
     *
     *  @returns see PlatformAdaptor_Abstract
     */
    QString currentSSID() override;

    /*! \brief Implements pure virtual method from PlatformAdaptor_Abstract */
    void disableScreenSaver() override;

    /*! \brief Implements pure virtual method from PlatformAdaptor_Abstract
     *
     *  @param lock see PlatformAdaptor_Abstract
     */
    void lockWifi(bool lock) override;

    /*! \brief Implements pure virtual method from PlatformAdaptor_Abstract */
    void vibrateBrief() override;

    /*! \brief Implements pure virtual method from PlatformAdaptor_Abstract */
    void vibrateLong() override;

    /*! \brief Implements pure virtual method from PlatformAdaptor_Abstract */
    void onGUISetupCompleted() override;

private slots:
    // This slot is called when the networkManagerInterface signals that the
    // network state has changed. It emits the signal wifiConnected as appropriate.
    void onNetworkStateChanged(uint);

private:
    Q_DISABLE_COPY_MOVE(PlatformAdaptor)

    // QDBusInterface to org.freedesktop.Notifications
    // This implementation of notifications uses the specification found here:
    // https://people.freedesktop.org/~lkundrak/nm-docs/spec.html
    //
    // Help with DBus programming is found here:
    // https://develop.kde.org/docs/d-bus/accessing_dbus_interfaces/
    QDBusInterface networkManagerInterface {QStringLiteral("org.freedesktop.NetworkManager"), QStringLiteral("/org/freedesktop/NetworkManager"), QStringLiteral("org.freedesktop.NetworkManager"), QDBusConnection::systemBus()};
};

} // namespace Platform
