/***************************************************************************
 *   Copyright (C) 2022-2025 by Stefan Kebekus                             *
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

import QtPositioning
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import akaflieg_freiburg.enroute

import "../dialogs"
import "../items"

Page {
    property bool isIos: Qt.platform.os === "ios"
    property bool isAndroid: Qt.platform.os === "android"
    property bool isAndroidOrIos: isAndroid || isIos


    id: page
    title: qsTr("Waypoint Library")
    focus: true

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

            icon.source: "/icons/material/ic_more_vert.svg"
            onClicked: {
                PlatformAdaptor.vibrateBrief()
                headerMenuX.open()
            }

            AutoSizingMenu {
                id: headerMenuX
                cascade: true

                MenuItem {
                    id: menuImport

                    text: qsTr("Import…")

                    onTriggered: {
                        PlatformAdaptor.vibrateBrief()
                        highlighted = false
                        if (isIos) {
                            Global.dialogLoader.active = false
                            Global.dialogLoader.setSource("../dialogs/LongTextDialog.qml", {
                                                              title: qsTr("Import files"),
                                                              text: qsTr("Locate your file in the browser, then select 'Open with' from the share menu, and choose Enroute"),
                                                              standardButtons: Dialog.Ok})
                            Global.dialogLoader.active = true
                        } else if (isAndroid) {
                            FileExchange.openFilePicker("")
                        } else {
                            importFileDialog.open()
                        }
                    }

                    FileDialog {
                        id: importFileDialog

                        acceptLabel: qsTr("Import")
                        rejectLabel: qsTr("Cancel")

                        fileMode: FileDialog.OpenFile

                        // Setting a non-trivial name filter on Android means we cannot select any
                        // files at all.
                        nameFilters: Qt.platform.os === "android" ? undefined : [qsTr("CUP File (*.cup *.txt)"),
                                                                                 qsTr("GeoJSON File (*.geojson *.json)"),
                                                                                 qsTr("GPX File (*.gpx)")]

                        onAccepted: {
                            PlatformAdaptor.vibrateBrief()
                            importFileDialog.close()
                            FileExchange.processFileOpenRequest(importFileDialog.selectedFile)
                        }
                        onRejected: {
                            PlatformAdaptor.vibrateBrief()
                            importFileDialog.close()
                        }
                    }
                }

                AutoSizingMenu {
                    title: Qt.platform.os === "android" ? qsTr("Share…") : qsTr("Export…")
                    enabled: WaypointLibrary.waypoints.length > 0

                    MenuItem {
                        text: qsTr("… to GeoJSON file")
                        onTriggered: {
                            headerMenuX.close()
                            PlatformAdaptor.vibrateBrief()
                            highlighted = false
                            parent.highlighted = false
                            var errorString = FileExchange.shareContent(WaypointLibrary.GeoJSON, "application/geo+json", qsTr("Waypoint Library"))
                            if (errorString === "abort") {
                                toast.doToast(qsTr("Aborted"))
                                return
                            }
                            if (errorString !== "") {
                                shareErrorDialog.text = errorString
                                shareErrorDialog.open()
                                return
                            }
                            if (isAndroidOrIos)
                                toast.doToast(qsTr("Waypoint library shared"))
                            else
                                toast.doToast(qsTr("Waypoint library exported"))
                        }
                    }

                    MenuItem {
                        text: qsTr("… to GPX file")
                        onTriggered: {
                            headerMenuX.close()
                            PlatformAdaptor.vibrateBrief()
                            highlighted = false
                            parent.highlighted = false
                            var errorString = FileExchange.shareContent(WaypointLibrary.toGpx(), "application/gpx+xml", qsTr("Waypoint Library"))
                            if (errorString === "abort") {
                                toast.doToast(qsTr("Aborted"))
                                return
                            }
                            if (errorString !== "") {
                                shareErrorDialog.text = errorString
                                shareErrorDialog.open()
                                return
                            }
                            if (isAndroidOrIos)
                                toast.doToast(qsTr("Waypoint library shared"))
                            else
                                toast.doToast(qsTr("Waypoint library exported"))
                        }
                    }
                }

                AutoSizingMenu {
                    title: qsTr("Open in Other App…")
                    enabled: (Qt.platform.os !== "ios") && (WaypointLibrary.waypoints.length > 0)

                    MenuItem {
                        text: qsTr("… in GeoJSON format")

                        onTriggered: {
                            PlatformAdaptor.vibrateBrief()
                            highlighted = false
                            parent.highlighted = false

                            var errorString = FileExchange.viewContent(WaypointLibrary.GeoJSON, "application/geo+json", "WaypointLibrary-%1.geojson")
                            if (errorString !== "") {
                                shareErrorDialog.text = errorString
                                shareErrorDialog.open()
                            } else
                                toast.doToast(qsTr("Waypoint library opened in other app"))
                        }
                    }

                    MenuItem {
                        text: qsTr("… in GPX format")

                        onTriggered: {
                            PlatformAdaptor.vibrateBrief()
                            highlighted = false
                            parent.highlighted = false

                            var errorString = FileExchange.viewContent(WaypointLibrary.toGpx(), "application/gpx+xml", "WaypointLibrary-%1.gpx")
                            if (errorString !== "") {
                                shareErrorDialog.text = errorString
                                shareErrorDialog.open()
                            } else
                                toast.doToast(qsTr("Waypoint library opened in other app"))
                        }
                    }

                }

                MenuSeparator { }

                MenuItem {
                    text: qsTr("Clear")
                    enabled: WaypointLibrary.waypoints.length > 0

                    onTriggered: {
                        PlatformAdaptor.vibrateBrief()
                        highlighted = false
                        clearDialog.open()
                    }

                }
            }
        }
    }

    Component {
        id: waypointDelegate

        RowLayout {
            width: wpList.width
            height: iDel.height

            SwipeToDeleteDelegate {
                id: iDel
                Layout.fillWidth: true

                text: modelData.name
                icon.source: modelData.icon

                onClicked: {
                    PlatformAdaptor.vibrateBrief()
                    waypointDescription.waypoint = modelData
                    waypointDescription.open()
                }

                swipe.onCompleted: {
                    PlatformAdaptor.vibrateBrief()
                    removeDialog.waypoint = modelData
                    removeDialog.open()
                }
            }

            ToolButton {
                id: editButton

                icon.source: "/icons/material/ic_mode_edit.svg"
                onClicked: {
                    PlatformAdaptor.vibrateBrief()
                    wpEditor.waypoint = modelData
                    wpEditor.open()
                }
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
                            removeDialog.waypoint = modelData
                            removeDialog.open()
                        }
                    } // removeAction
                } // AutoSizingMenu

            }

        }

    }


    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: SafeInsets.left
        anchors.rightMargin: SafeInsets.right

        Item {
            Layout.preferredHeight: textInput.font.pixelSize/4.0
        }


        MyTextField {
            id: textInput

            Layout.fillWidth: true
            Layout.leftMargin: font.pixelSize/2.0
            Layout.rightMargin: font.pixelSize/2.0

            placeholderText: qsTr("Filter by Name")
        }

        DecoratedListView {
            id: wpList

            Layout.fillWidth: true
            Layout.fillHeight: true

            clip: true

            model:
            Binding {
                wpList.model: {
                    // Mention waypoints to ensure that the list gets updated
                    WaypointLibrary.waypoints

                    return WaypointLibrary.filteredWaypoints(textInput.displayText)
                }
                delayed: true
            }

            delegate: waypointDelegate
            ScrollIndicator.vertical: ScrollIndicator {}
        }

        Label {
            Layout.fillWidth: true
            Layout.fillHeight: true

            visible: (wpList.count === 0)
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            leftPadding: font.pixelSize*2
            rightPadding: font.pixelSize*2

            textFormat: Text.RichText
            wrapMode: Text.Wrap
            text: (textInput.text === "")
                  ? qsTr("<h3>Sorry!</h3><p>No waypoint available. To add a waypoint here, choose 'Add Waypoint' below or double-tap on a point in the moving map.</p>")
                  : qsTr("<h3>Sorry!</h3><p>No waypoints match your filter.</p>")
        }

    }



    footer: Footer {
        ColumnLayout {
            width: parent.width

            Button {
                id: addWPButton

                flat: true

                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Add Waypoint")
                icon.source: "/icons/material/ic_add_circle.svg"

                onClicked: {
                    PlatformAdaptor.vibrateBrief()
                    addWP.open()
                }
            }
        }
    }

    // This is the name of the file that openFromLibrary will open
    property string finalFileName;

    function reloadWaypointList() {
        var cache = textInput.text
        textInput.text = textInput.text+"XXXXX"
        textInput.text = cache
    }

    LongTextDialog {
        id: shareErrorDialog

        title: qsTr("Error Exporting Data…")
        standardButtons: Dialog.Ok
    }

    LongTextDialog {
        id: removeDialog

        property var waypoint: GeoMapProvider.createWaypoint()

        title: qsTr("Remove from Device?")
        text: qsTr("Once the waypoint <strong>%1</strong> is removed, it cannot be restored.").arg(removeDialog.waypoint.name)

        standardButtons: Dialog.No | Dialog.Yes

        onAccepted: {
            PlatformAdaptor.vibrateBrief()
            WaypointLibrary.remove(removeDialog.waypoint)
            page.reloadWaypointList()
            toast.doToast(qsTr("Waypoint removed from device"))
        }
        onRejected: {
            PlatformAdaptor.vibrateBrief()
            page.reloadWaypointList() // Re-display aircraft that have been swiped out
            close()
        }
    }

    LongTextDialog {
        id: clearDialog

        title: qsTr("Clear Waypoint Library?")
        standardButtons: Dialog.No | Dialog.Yes

        text: qsTr("Once cleared, the library cannot be restored.")

        onAccepted: {
            PlatformAdaptor.vibrateBrief()
            WaypointLibrary.clear()
            page.reloadWaypointList()
            toast.doToast(qsTr("Waypoint library cleared"))
        }
    }

    WaypointEditor {
        id: wpEditor

        onAccepted: {
            let newWP = waypoint.copy()
            newWP.name = newName
            newWP.notes = newNotes
            newWP.coordinate = QtPositioning.coordinate(newLatitude, newLongitude, newAltitudeMeter)
            WaypointLibrary.replace(waypoint, newWP)
            page.reloadWaypointList()
            toast.doToast(qsTr("Waypoint modified"))
        }

    }

    WaypointEditor {
        id: addWP

        title: qsTr("Add Waypoint")

        onAccepted: {
            let newWP = waypoint.copy()
            newWP.name = newName
            newWP.notes = newNotes
            newWP.coordinate = QtPositioning.coordinate(newLatitude, newLongitude, newAltitudeMeter)
            WaypointLibrary.add(newWP)
            page.reloadWaypointList()
            toast.doToast(qsTr("Waypoint added"))
        }

    }

    WaypointDescription {
        id: waypointDescription
    }

} // Page
