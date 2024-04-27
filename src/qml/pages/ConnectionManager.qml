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

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import akaflieg_freiburg.enroute
import "../dialogs"
import "../items"

Page {
    id: trafficReceiverPage
    objectName: "TrafficReceiverPage"

    title: qsTr("Settings: Traffic Data Receivers")

    required property var appWindow

    header: StandardHeader {}


    DecoratedListView {
        anchors.fill: parent
        contentWidth: availableWidth // Disable horizontal scrolling

        clip: true

        model: TrafficDataProvider.dataSources()

        delegate: Item {
            width: parent ? parent.width : 0
            height: idel.implicitHeight

            Rectangle {
                anchors.fill: parent
                color: {
                    if (model.modelData.receivingHeartbeat)
                        return "green"
                    return "transparent"
                }
                opacity: 0.2
            }

            RowLayout {
                width: parent.width

                WordWrappingItemDelegate {
                    id: idel
                    Layout.fillWidth: true

                    //enabled: model.modelData.canConnect
                    icon.source: model.modelData.icon
                    text: model.modelData.sourceName + "<br><font size='2'>%1</font>".arg(model.modelData.connectivityStatus)
                }

                ToolButton {
                    id: cptMenuButton

                    icon.source: "/icons/material/ic_more_horiz.svg"

                    onClicked: {
                        PlatformAdaptor.vibrateBrief()
                        cptMenu.open()
                    }

                    AutoSizingMenu {
                        id: cptMenu

                        Action {
                            id: removeAction
                            text: qsTr("Remove…")
                            onTriggered: {
                                PlatformAdaptor.vibrateBrief()
                            }
                        }
                    }
                }
            }

        }
    }

    footer: Footer {
        ColumnLayout {
            width: parent.width

            Button {
                flat: true

                Layout.alignment: Qt.AlignHCenter
                icon.source: "/icons/material/ic_tap_and_play.svg"
                text: qsTr("Reconnect")
                enabled: !timer.running
                visible: !TrafficDataProvider.receivingHeartbeat
                onClicked: {
                    TrafficDataProvider.connectToTrafficReceiver()
                    timer.running = true;
                }
                Timer {
                    id: timer
                    interval: 1000
                }
            }

            Button {
                flat: true

                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Add Bluetooth Device")
                icon.source: "/icons/material/ic_add_circle.svg"

                onClicked: {
                    PlatformAdaptor.vibrateBrief()
                    Global.dialogLoader.active = false
                    Global.dialogLoader.setSource("../dialogs/AddBTDeviceDialog.qml", {})
                    Global.dialogLoader.active = true
                }
            }
        }
    }

}