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

#pragma once

#include <QGeoCoordinate>
#include <QQmlEngine>
#include <QQuickItem>

#include "GlobalObject.h"


namespace Platform {

/*! \brief Interface to platform-specific functionality
 *
 * This pure virtual class is an interface to capabilities of mobile devices
 * (e.g. vibration) that need platform-specific code to operate.  The files
 * PlatformAdaptor_XXX.(h|cpp) implement a child class PlatformAdaptor that
 * contains the actual implementation.
 *
 * Child classes need to implement all pure virtual functions, and need to
 * provide the following additional functionality.
 *
 * - Child classes need to monitor the Wi-Fi network and emit the signal
 *   wifiConnected() whenever a new Wi-Fi connection becomes available. The app
 *   will then check if a traffic data receiver is active in the network.
 *
 * - If supported by the platform, child classes need to react to requests by
 *   the platform to open a file (e.g. a GeoJSON file containing a flight
 *   route). Once a request is received, the method processFileRequest() should
 *   be called.
 */

class PlatformAdaptor_Abstract : public GlobalObject
{
    Q_OBJECT

public:
    /*! \brief Standard constructor
     *
     * @param parent Standard QObject parent pointer
    */
    explicit PlatformAdaptor_Abstract(QObject* parent = nullptr);

    // No default constructor, important for QML singleton
    explicit PlatformAdaptor_Abstract() = delete;

    ~PlatformAdaptor_Abstract() override = default;

    // factory function for QML singleton
    static Platform::PlatformAdaptor_Abstract* create(QQmlEngine* /*unused*/, QJSEngine* /*unused*/)
    {
        return GlobalObject::platformAdaptor();
    }


    //
    // Methods
    //

    /*! \brief Content of ClipBoard
     *
     *  @returns Text content of the clipboard
     */
    Q_INVOKABLE static QString clipboardText();

    /*! \brief SSID of current Wi-Fi network
     *
     * @returns The SSID of the current Wi-Fi networks, an empty string if the
     * device is not connected to a Wi-Fi or a generic string if the SSID cannot
     * be determined.
     */
    Q_INVOKABLE virtual QString currentSSID() { return {}; }

    /*! \brief Disable the screen saver
     *
     * On platforms that support this, this method shall disable to screen
     * saver, so that the display does not switch off automatically.  This is
     * meant to ensure that the display remains on while the app is in use (e.g.
     * while the pilot is following a non-standard traffic pattern).
     */
    Q_INVOKABLE virtual void disableScreenSaver() {}

    /*! \brief Lock connection to Wi-Fi network
     *
     * If supported by the platform, this method is supposed to lock the current
     * Wi-Fi connection, that is, to prevent the device from dropping the
     * connection or shutting down the Wi-Fi interface.
     *
     * The app calls that method after connecting to a traffic data receiver, in
     * order to ensure that traffic data is continuously received.
     *
     * @param lock If true, then lock the network. If false, then release the
     * lock.
     */
    Q_INVOKABLE virtual void lockWifi(bool lock) { Q_UNUSED(lock) }

    /*! \brief Open an external app or web site showing a satellite view
     *
     *  This implementation uses QDesktopServices::openUrl() to open the
     *  location in Google Maps, either in a native app or in an external web
     *  browser winde. The Android implementation tries Google Earth first and
     *  falls back to Google Maps if Google Earth is not installed.
     *
     * @param coordinate Location whose sat view should be shown.
     */
    Q_INVOKABLE virtual void openSatView(const QGeoCoordinate& coordinate);

    /*! \brief Workaround for QTBUG-80790
     *
     *  This method is empty except on iOS, where it installs a special event
     *  filter for a QQuickItem. The event filter avoids problematic behavior
     *  where the virtual keyboard pushes up the whole qml page. Details for
     *  this problem and the present workaround are described here.
     *
     *  https://stackoverflow.com/questions/34716462/ios-sometimes-keyboard-pushes-up-the-whole-qml-page
     *
     *  https://bugreports.qt.io/browse/QTBUG-80790
     *
     *  @param item QQuickItem where the event filter is to be intalled.
     */
    Q_INVOKABLE virtual void setupInputMethodEventFilter(QQuickItem* item) { Q_UNUSED(item) }

    /*! \brief Information about the system, in HTML format
     *
     * @returns Info string
     */
    Q_INVOKABLE virtual QString systemInfo();

    /*! \brief Make the device briefly vibrate
     *
     *  On platforms that support this, make the device briefly vibrate if
     *  haptic feedback is enabled in the system settings.
     */
    Q_INVOKABLE virtual void vibrateBrief() {}

    /*! \brief Make the device vibrate for a longer period
     *
     *  On platforms that support this, make the device vibrate for a longer time,
     *  if haptic feedback is enabled in the system settings. This is used to
     *  when showing notifications.
     */
    Q_INVOKABLE virtual void vibrateLong() {}

    /*! \brief Language code that is to be used in the GUI
     *
     *  @returns A two-letter language code (such as "en" or "de") that
     *  describes the language that is to be used in the GUI.
     */
    Q_INVOKABLE virtual QString language();

    /*! \brief Save image as a raster graphic file in path
     *
     *  The image file type (PNG, JPG, …) is inferred from the file path
     *
     *  @param image Image to be saved.
     *
     *  @param path File path where the image will be stored
     */
    Q_INVOKABLE static void saveScreenshot(const QImage& image, const QString& path);

public slots:
    /*! \brief Signal handler: GUI setup completed
     *
     *  This method is called as soon as the GUI setup is completed. On Android,
     *  this method is used to hide the splash screen and to show the app.
     *
     *  The implementation should guarentee that nothing bad happens if the
     *  method is called more than once.
     */
    virtual void onGUISetupCompleted() {}


signals:
    /*! \brief Emitted when an error occurs
     *
     *  This signal is emitted when an error occurs. The GUI will show the
     *  message in an appropriate dialog.
     *
     *  @param message Human-readable, translated message
     */
    void error(const QString& message);

    /*! \brief Emitted when the OS requests a language change
     *
     *  This signal is emitted when the OS requests a language change. The GUI
     *  will show a dialog requesting the user to restart the app.
     */
    void languageChanged();

    /*! \brief Emitted when a new WiFi connections becomes available
     *
     *  This signal is emitted when a new WiFi connection becomes available.
     */
    void wifiConnected();

private:
    Q_DISABLE_COPY_MOVE(PlatformAdaptor_Abstract)

};

} // namespace Platform
