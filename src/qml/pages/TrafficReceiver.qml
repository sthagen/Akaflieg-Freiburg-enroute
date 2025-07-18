/***************************************************************************
 *   Copyright (C) 2019-2025 by Stefan Kebekus                             *
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

    title: qsTr("Traffic Data Receiver")

    required property var appWindow


    header: PageHeader {

        height: 60 + SafeInsets.top
        leftPadding: SafeInsets.left
        rightPadding: SafeInsets.right
        topPadding: SafeInsets.top

        ToolButton {
            id: backButton

            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter

            icon.source: "/icons/material/ic_arrow_back.svg"

            onClicked: {
                PlatformAdaptor.vibrateBrief()
                stackView.pop()
            }
        }

        Label {
            id: lbl

            anchors.verticalCenter: parent.verticalCenter

            anchors.left: parent.left
            anchors.leftMargin: 72
            anchors.right: headerMenuToolButton.left

            text: stackView.currentItem.title
            elide: Label.ElideRight
            font.pixelSize: 20
            verticalAlignment: Qt.AlignVCenter
        }

        ToolButton {
            id: headerMenuToolButton

            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter

            icon.source: "/icons/material/ic_info_outline.svg"
            onClicked: {
                PlatformAdaptor.vibrateBrief()
                openManual("05-referencePages/04-infoTraffic.html")
            }
        }
    }


    Component {
        id: trafficDelegate

        Item {
            width: sView.width
            height: idel.height

            Rectangle {
                anchors.fill: parent
                color: model.modelData.color
                opacity: 0.2
            }

            WordWrappingItemDelegate {
                width: parent.width
                leftPadding: SafeInsets.left+16
                rightPadding: SafeInsets.right+16

                id: idel
                text: {
                    var line1 = []
                    if (model.modelData.callSign !== "")
                        line1.push(model.modelData.callSign)
                    line1.push(model.modelData.typeString)
                    if (model.modelData.ID !== "")
                        line1.push("ID: " + model.modelData.ID)

                    var line2 = []
                    if (model.modelData.hDist.isFinite())
                        line2.push(qsTr("hDist") + ": " + Navigator.aircraft.horizontalDistanceToString(model.modelData.hDist))
                    if (model.modelData.vDist.isFinite())
                        line2.push(qsTr("vDist") + ": " + Navigator.aircraft.verticalDistanceToString(model.modelData.vDist))
                    return "<strong>" + line1.join(" • ") + "</strong><br><font size='2'>" + line2.join(" • ") + "</font>"
                }
                icon.source: "/icons/material/ic_airplanemode_active.svg"
            }
        }
    }

    TrafficObserver {
        id: trafficObserver
    }

    DecoratedScrollView {
        id: sView

        anchors.fill: parent
        contentWidth: availableWidth // Disable horizontal scrolling

        clip: true

        bottomPadding: font.pixelSize + SafeInsets.bottom
        leftPadding: font.pixelSize + SafeInsets.left
        rightPadding: font.pixelSize + SafeInsets.right
        topPadding: font.pixelSize

        ColumnLayout {
            width: sView.availableWidth

            Label {
                Layout.fillWidth: true

                text: qsTr("Connection Status")
                font.pixelSize: sView.font.pixelSize*1.2
                font.bold: true
            }

            Label { // Status
                Layout.fillWidth: true
                Layout.leftMargin: 4
                Layout.rightMargin: 4

                text: TrafficDataProvider.statusString

                wrapMode: Text.WordWrap
                textFormat: Text.RichText

                bottomPadding: 0.6*font.pixelSize
                topPadding: 0.6*font.pixelSize
                leftPadding: 0.2*font.pixelSize
                rightPadding: 0.2*font.pixelSize

                leftInset: -4
                rightInset: -4

                background: Rectangle {
                    border.color: "black"
                    color: {
                        if (!TrafficDataProvider.receivingHeartbeat)
                            return "red"
                        if (TrafficDataProvider.currentSourceIsInternetService)
                            return "yellow"
                        return "green"
                    }
                    opacity: 0.2
                    radius: 4
                }
            }

            Label {
                Layout.fillWidth: true
                visible: TrafficDataProvider.receivingHeartbeat

                text: qsTr("Traffic Data Receiver Status")
                font.pixelSize: sView.font.pixelSize*1.2
                font.bold: true
            }

            Label {
                id: problemStatus

                Layout.fillWidth: true
                Layout.leftMargin: 4
                Layout.rightMargin: 4

                visible: TrafficDataProvider.receivingHeartbeat

                bottomPadding: 0.6*font.pixelSize
                topPadding: 0.6*font.pixelSize
                leftPadding: 0.2*font.pixelSize
                rightPadding: 0.2*font.pixelSize

                leftInset: -4
                rightInset: -4

                property string myText: {
                    if (TrafficDataProvider.trafficReceiverRuntimeError === "")
                        return TrafficDataProvider.trafficReceiverSelfTestError
                    if (TrafficDataProvider.trafficReceiverSelfTestError === "")
                        return TrafficDataProvider.trafficReceiverRuntimeError
                    return TrafficDataProvider.trafficReceiverRuntimeError + "<br>" + TrafficDataProvider.trafficReceiverSelfTestError
                }

                text: (myText === "") ? qsTr("No problem reported") : myText
                wrapMode: Text.WordWrap

                background: Rectangle {
                    border.color: "black"
                    color: (problemStatus.myText === "") ? "green" : "red"
                    opacity: 0.2
                    radius: 4
                }

            }

            Label {
                Layout.fillWidth: true
                visible: TrafficDataProvider.receivingHeartbeat

                text: qsTr("Traffic")
                font.pixelSize: sView.font.pixelSize*1.2
                font.bold: true
            }

            Label {
                Layout.fillWidth: true
                visible: TrafficDataProvider.receivingHeartbeat && !trafficObserver.hasTraffic

                text: qsTr("Currently No Traffic")
            }

            ListView {
                Layout.fillWidth: true
                Layout.preferredHeight: contentHeight
                clip: true

                model: trafficObserver.traffic
                delegate: trafficDelegate
                ScrollIndicator.vertical: ScrollIndicator {}

                section.property: "modelData.relevantString"
                section.delegate: Component {
                    Control {
                        required property string section

                        height: lbl.height

                        Label {
                            id: lbl

                            //x: font.pixelSize
                            text: parent.section
                            //font.pixelSize: parent.font.pixelSize*1.2
                            //font.bold: true
                        }
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                visible: !TrafficDataProvider.receivingHeartbeat

                text: qsTr("Help")
                font.pixelSize: sView.font.pixelSize*1.2
                font.bold: true
            }

            WordWrappingItemDelegate {
                Layout.fillWidth: true
                visible: !TrafficDataProvider.receivingHeartbeat
                icon.source: "/icons/material/ic_info_outline.svg"
                text: qsTr("Connect to a traffic receiver…")
                onClicked: trafficReceiverPage.appWindow.openManual("02-senseAndAvoid.html")
            }

            WordWrappingItemDelegate {
                Layout.fillWidth: true
                visible: !TrafficDataProvider.receivingHeartbeat
                icon.source: "/icons/material/ic_info_outline.svg"
                text: qsTr("Connect to a flight simulator…")
                onClicked: trafficReceiverPage.appWindow.openManual("02-tutorialBasic/07-simulator.html")
            }
        }

    }

    footer: Footer {
        ColumnLayout {
            width: parent.width

            Button {
                flat: true

                Layout.alignment: Qt.AlignHCenter
                icon.source: "/icons/material/ic_refresh.svg"
                text: {
                    if (disconnectTimer.running)
                        return qsTr("Disconnecting...")
                    if (connectTimer.running)
                        return qsTr("Reconnecting...")
                    return qsTr("Reconnect")
                }
                enabled: !connectTimer.running
                visible: !TrafficDataProvider.receivingHeartbeat
                onClicked: {
                    TrafficDataProvider.disconnectFromTrafficReceiver()
                    disconnectTimer.running = true;
                    connectTimer.running = true;
                }
                Timer {
                    id: disconnectTimer
                    interval: 1000
                    onTriggered: TrafficDataProvider.connectToTrafficReceiver()
                }
                Timer {
                    id: connectTimer
                    interval: 2000
                }
            }

            Button {
                flat: true

                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Configure Data Connections")
                icon.source: "/icons/material/ic_tap_and_play.svg"

                onClicked: {
                    PlatformAdaptor.vibrateBrief()
                    stackView.push("ConnectionManager.qml", {"appWindow": view})
                }
            }
        }
    }
}
