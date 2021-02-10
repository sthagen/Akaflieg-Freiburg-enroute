/***************************************************************************
 *   Copyright (C) 2019-2020 by Stefan Kebekus                             *
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

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import enroute 1.0

Dialog {
    id: flarmStatusDialog

    title: qsTr("Traffic Receiver")

    // Size is chosen so that the dialog does not cover the parent in full
    // Size is chosen so that the dialog does not cover the parent in full
    width: Math.min(parent.width-Qt.application.font.pixelSize, 40*Qt.application.font.pixelSize)
    height: Math.min(parent.height-Qt.application.font.pixelSize, implicitHeight)

    standardButtons: Dialog.Ok

    ScrollView {
        id: view
        clip: true
        anchors.fill: parent

        // The visibility behavior of the vertical scroll bar is a little complex.
        // The following code guarantees that the scroll bar is shown initially. If it is not used, it is faded out after half a second or so.
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: (height < contentHeight) ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded

        GridLayout {
            id: gl
            columnSpacing: 30
            columns: 2

            width: flarmStatusDialog.availableWidth

            Text { // Status
                Layout.alignment: Qt.AlignTop
                text: qsTr("Status")
            }
            Text {
                Layout.fillWidth: true
                font.weight: Font.Bold
                text: flarmAdaptor.statusString
                color: (flarmAdaptor.status === FLARMAdaptor.OK) ? "green" : "red"
                wrapMode: Text.Wrap
            }

            Text { // Activity
                Layout.alignment: Qt.AlignTop
                text: qsTr("Activity")
            }
            Text {
                Layout.fillWidth: true
                text: flarmAdaptor.activity
                wrapMode: Text.Wrap
            }

            Text {
                Layout.alignment: Qt.AlignTop
                text: qsTr("Last Error")
            }
            Text {
                Layout.fillWidth: true
                text: flarmAdaptor.lastError
                wrapMode: Text.Wrap
            }

            Text {
                Layout.alignment: Qt.AlignTop
                text: qsTr("Hardware")
            }
            Text {
                Layout.fillWidth: true
                text: flarmAdaptor.FLARMHwVersion
                wrapMode: Text.Wrap
            }

            Text {
                Layout.alignment: Qt.AlignTop
                text: qsTr("Software")
            }
            Text {
                Layout.fillWidth: true
                text: flarmAdaptor.FLARMSwVersion
                wrapMode: Text.Wrap
            }

            Text {
                Layout.alignment: Qt.AlignTop
                text: qsTr("Obstacle DB")
            }
            Text {
                Layout.fillWidth: true
                text: flarmAdaptor.FLARMObstVersion
                wrapMode: Text.Wrap
            }

            Text {
                Layout.alignment: Qt.AlignTop
                text: qsTr("Self test")
            }
            Text {
                Layout.fillWidth: true
                text: flarmAdaptor.FLARMSelfTest
                wrapMode: Text.Wrap
            }

            Button {
                Layout.columnSpan: 2
                Layout.alignment: Qt.AlignHCenter
                text: flarmAdaptor.canConnect ? qsTr("Manually Connect") :  qsTr("Manually Disconnect")
                onClicked: flarmAdaptor.canConnect ? flarmAdaptor.connectToFLARM() : flarmAdaptor.disconnectFromFLARM()
            }

        } // GridLayout

    } // Scrollview

    onAccepted: {
        // Give feedback
        mobileAdaptor.vibrateBrief()
        close()
    }
} // Dialog
