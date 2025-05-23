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

#include "PlatformAdaptor_Abstract.h"
#include "qcoreevent.h"
#include "qevent.h"
#include "qquickitem.h"

namespace Platform {


class ImFixer : public QObject {
    Q_OBJECT
protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::InputMethodQuery) {
            auto* imEvt = static_cast<QInputMethodQueryEvent *>(event);
            if (imEvt->queries() == Qt::InputMethodQuery::ImCursorRectangle) {
                imEvt->setValue(Qt::InputMethodQuery::ImCursorRectangle, QRectF());
                return true;
            }
        }
        return QObject::eventFilter(obj, event);
    }
};

/*! \brief Template implementation of PlatformAdaptor */

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
    explicit PlatformAdaptor(QObject *parent = nullptr);

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

    /*! \brief Implements virtual method from PlatformAdaptor_Abstract,
     *  workaround for QTBUG-80790
     *
     *  @param item QQuickItem where the event filter is to be intalled.
     */
    void setupInputMethodEventFilter(QQuickItem *item) override {
        static thread_local ImFixer imf;
        item->installEventFilter(&imf);
    }

    /*! \brief Implements pure virtual method from PlatformAdaptor_Abstract */
    void vibrateBrief() override;

    /*! \brief Implements pure virtual method from PlatformAdaptor_Abstract */
    void vibrateLong() override;

    /*! \brief Reimplements virtual method from PlatformAdaptor_Abstract
     *
     *  @returns A two-letter language code, as documented in PlatformAdaptor_Abstract
     */
    QString language() override;

#warning documentation
    static Q_INVOKABLE void saveScreenshot(const QImage &, QString);

    /*! \brief Implements pure virtual method from PlatformAdaptor_Abstract */
    void onGUISetupCompleted() override;

private:
    Q_DISABLE_COPY_MOVE(PlatformAdaptor)
};

} // namespace Platform
