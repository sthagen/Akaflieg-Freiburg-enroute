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

#include <QtGlobal>
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)

#include <QObject>
#include <QTimer>

#include "PlatformAdaptor_Abstract.h"

namespace Platform {

/*! \brief Interface to platform-specific capabilities of mobile devices
 *
 * This class is an interface to capabilities of mobile devices (e.g. vibration)
 * that need platform-specific code to operate.
 *
 * This class also receives platform-specific requests to open files and exposes
 * these requests to C++ and QML via the signal openFileRequest().  The signal
 * openFileRequest() is however only emitted after the method
 * startReceiveOpenFileRequests() has been called, requests that come in earlier
 * will be put on hold.  This allows apps to set up their GUI before processing
 * file open requests (that might need user interaction).
 */

class PlatformAdaptor : public PlatformAdaptor_Abstract
{
    Q_OBJECT

public:
    /*! \brief Standard constructor
     *
     * On Android, this constructor disables the screen lock and asks for the
     * permissions that are needed to run the app.
     *
     * @param parent Standard QObject parent pointer
    */
    explicit PlatformAdaptor(QObject *parent = nullptr);

    ~PlatformAdaptor() override = default;


    //
    // Methods
    //

    /*! \brief SSID of current Wi-Fi network
     *
     * @returns The SSID of the current Wi-Fi networks, or an empty of generic string
     * if the device is not connected to a Wi-Fi or if the SSID could not be determined.
     */
    Q_INVOKABLE QString currentSSID() override;

    /*! \brief Checks if all required permissions have been granted
     *
     * On Android, the app requirs certain permissions to run. This method can
     * be used to check if all permissions have been granted.
     *
     * @returns On Android, returns 'true' if not all required permissions have
     * been granted. On other systems, always returns 'false'
    */
    Q_INVOKABLE bool hasMissingPermissions() override;

    /*! \brief Import content from file
     *
     * On Android systems, this method does nothing.
     *
     * On other desktop systems, this method uses a file dialog to import a file.
     */
    Q_INVOKABLE void importContent() override;

    /*! \brief Lock connection to Wi-Fi network
     *
     * Under Android, this method can lock the Wi-Fi connection by acquiring a
     * WifiManager.WifiLock. On other platforms, this method does nothing.
     *
     * @param lock If true, then lock the network. If false, then release the lock.
     */
    Q_INVOKABLE void lockWifi(bool lock) override;

    /*! \brief Share content to file or to file sending app
     *
     * On Android systems, this method will save content to temporary file in
     * the app's private cache and call the corresponding java static method
     * where a SEND intent is created and startActivity is called
     *
     * On other desktop systems, this method uses a file dialog to save the file.
     *
     * @param content content text
     *
     * @param mimeType the mimeType of the content
     *
     * @param fileNameTemplate A string of the form "EDTF - EDTG", without suffix of path. This
     * file name is visible to the user. It appears for instance as the name of
     * the attachment when sending files by e-mail.
     *
     * @returns Empty string on success, the string "abort" on abort, and a translated error message otherwise
     */
    Q_INVOKABLE QString shareContent(const QByteArray& content, const QString& mimeType, const QString& fileNameTemplate) override;

    /*! \brief Make the device briefly vibrate
     *
     * On Android, make the device briefly vibrate if haptic feedback is enabled in the system settings.
     *
     * On other platforms, this does nothing.
    */
    Q_INVOKABLE void vibrateBrief() override;

    /*! \brief View content in other app
     *
     * On Android systems, this method will save content to temporary file in
     * the app's private cache and call the corresponding java static method
     * where a SEND intent is created and startActivity is called.
     *
     * On other systems, this method opens the file using QDesktopServices.
     *
     * @param content content text
     *
     * @param mimeType the mimeType of the content
     *
     * @param fileNameTemplate A string of the form "EDTF - EDTG", without suffix of path. This
     * file name is visible to the user. It appears for instance as the name of
     * the attachment when sending files by e-mail.
     *
     * @returns Empty string on success, a translated error message otherwise
     */
    Q_INVOKABLE QString viewContent(const QByteArray& content, const QString& mimeType, const QString& fileNameTemplate) override;

    // ------------------

    /*! \brief Hides the android splash screen.
     *
     * On Android, hides the android splash screen.
     *
     * On other platforms, this does nothing. The implementation ensures that
     * QtAndroid::hideSplashScreen is called (only once, regardless of how often
     * this slot is used).
    */
    Q_INVOKABLE void onGUISetupCompleted() override;

#warning
    virtual void requestPermissionsSync();

#warning
    void disableScreenSaver() override;

private:
    Q_DISABLE_COPY_MOVE(PlatformAdaptor)
};

} // namespace Platform

#endif // defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
