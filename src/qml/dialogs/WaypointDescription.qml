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

import QtPositioning
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes

import akaflieg_freiburg.enroute

pragma ComponentBehavior: Bound


/* This is a dialog with detailed information about a waypoint. To use this dialog, all you have to do is to set a Waypoint in the property "waypoint" and call open(). */

CenteringDialog {
    id: waypointDescriptionDialog

    property waypoint waypoint

    onWaypointChanged : {
        WeatherDataProvider.requestUpdate4Waypoint(waypoint)

        // Delete old text items
        let childCount = co.children.length;
        // Iterate through the children in reverse order
        var i
        for (i = childCount - 1; i >= 0; i--) {
            // Check if the child is a valid QML item
            if (co.children[i] instanceof QtObject) {
                    // Destroy the child item
                    co.children[i].destroy();
                }
            }

        // If no waypoint is given, then do nothing
        if (!waypoint.isValid)
            return

        // Create METAR info box
        metarInfo.createObject(co);

        // Create NOTAM info box
        notamInfo.createObject(co);

        // Create waypoint description items
        var pro = waypoint.tabularDescription
        for (var j in pro)
            waypointPropertyDelegate.createObject(co, {text: pro[j]});

        // Create airspace description items
        var asl = GeoMapProvider.airspaces(waypoint.coordinate)
        for (i in asl)
            airspaceDelegate.createObject(co, {airspace: asl[i]});

        // Create airspace description items
        var vac = VACLibrary.vacs4Point(waypoint.coordinate)
        for (i in vac)
            vacButtonDelegate.createObject(co, {vac: vac[i]});

        satButtonDelegate.createObject(co, {});
    }

    modal: true
    standardButtons: Dialog.Close
    focus: true

    title: {
        if (waypoint.ICAOCode === "")
            return waypoint.extendedName
        return waypoint.ICAOCode + " • " +waypoint.extendedName
    }


    Component {
        id: metarInfo

        Label { // METAR info
            Loader {
                id: secondaryDlgLoader
                onLoaded: item.open()
            }
            Observer {
                id: obs
                waypoint: waypointDescriptionDialog.waypoint
            }

            visible:  obs.metar.isValid || obs.taf.isValid
            text: {
                if (obs.metar.isValid)
                    return obs.metar.summary(Navigator.aircraft, Clock.time) + " • <a href='xx'>" + qsTr("full report") + "</a>"
                return "<a href='xx'>" + qsTr("read TAF") + "</a>"
            }
            Layout.fillWidth: true
            wrapMode: Text.WordWrap

            bottomPadding: 0.2*font.pixelSize
            topPadding: 0.2*font.pixelSize
            leftPadding: 0.2*font.pixelSize
            rightPadding: 0.2*font.pixelSize
            onLinkActivated: {
                PlatformAdaptor.vibrateBrief()
                secondaryDlgLoader.setSource("../dialogs/MetarTafDialog.qml", {"weatherStation": obs})
            }

            // Background color according to METAR/FAA flight category
            background: Rectangle {
                border.color: "black"
                color: obs.metar.flightCategoryColor
                opacity: 0.2
            }
        }
    }

    Component {
        id: notamInfo

        Label { // NOTAM info

            Loader {
                // WARNING This does not really belong here.
                id: dlgLoader
                onLoaded: item.open()
            }

            property notamList notamList: {
                // Mention lastUpdate, so we update whenever there is new data
                NOTAMProvider.lastUpdate
                return NOTAMProvider.notams(waypointDescriptionDialog.waypoint)
            }

            visible: text !== ""
            text: {
                if (notamList.isValid && notamList.isEmpty)
                    return ""
                if (notamList.isEmpty)
                    return notamList.summary
                return notamList.summary + " • <a href='xx'>" + qsTr("full report") + "</a>"
            }

            Layout.fillWidth: true
            wrapMode: Text.WordWrap

            bottomPadding: 0.2*font.pixelSize
            topPadding: 0.2*font.pixelSize
            leftPadding: 0.2*font.pixelSize
            rightPadding: 0.2*font.pixelSize
            onLinkActivated: {
                PlatformAdaptor.vibrateBrief()
                dlgLoader.setSource("../dialogs/NotamListDialog.qml", {"notamList": notamList, "waypoint": waypointDescriptionDialog.waypoint})
            }

            // Background color according to METAR/FAA flight category
            background: Rectangle {
                border.color: "black"
                color: "yellow"
                opacity: 0.2
            }

        }

    }

    Component {
        id: waypointPropertyDelegate

        RowLayout {
            id: rowLYO

            Layout.preferredWidth: sv.width

            property var text: ({});

            Label {
                text: rowLYO.text.substring(0,4)
                Layout.preferredWidth: font.pixelSize*3
                Layout.alignment: Qt.AlignTop
                font.bold: true

            }
            Label {
                Layout.fillWidth: true
                text: rowLYO.text.substring(4)
                wrapMode: Text.WordWrap
                textFormat: Text.StyledText
            }
        }
    }

    Component {
        id: airspaceDelegate

        GridLayout {
            id: gridLYO

            columns: 3
            rowSpacing: 0

            Layout.preferredWidth: sv.width

            property var airspace: ({});


            Item {
                id: box

                Layout.preferredWidth: colorGlean.font.pixelSize*3
                Layout.preferredHeight: colorGlean.font.pixelSize*2.5
                Layout.rowSpan: 3
                Layout.alignment: Qt.AlignLeft

                Shape {
                    anchors.fill: parent

                    ShapePath {
                        strokeWidth: 2
                        fillColor: "transparent"
                        strokeColor:  {
                            switch(gridLYO.airspace.CAT) {
                            case "A":
                            case "B":
                            case "C":
                            case "D":
                            case "E":
                            case "F":
                            case "G":
                                return "blue";
                            case "CTR":
                                return "blue";
                            case "GLD":
                                return "yellow";
                            case "DNG":
                            case "P":
                            case "PJE":
                            case "R":
                                return "red";
                            case "ATZ":
                            case "RMZ":
                            case "TIZ":
                            case "TIA":
                                return "blue";
                            case "TMZ":
                                return "black";
                            case "FIR":
                            case "FIS":
                            case "NRA":
                                return "green";
                            case "SUA":
                                return "red";
                            }
                            return "transparent"
                        }
                        strokeStyle:  {
                            switch(gridLYO.airspace.CAT) {
                            case "A":
                            case "B":
                            case "C":
                            case "D":
                            case "E":
                            case "F":
                            case "G":
                            case "GLD":
                            case "NRA":
                                return ShapePath.SolidLine;
                            }
                            return ShapePath.DashLine
                        }
                        dashPattern:  {
                            switch(gridLYO.airspace.CAT) {
                            case "TMZ":
                                return [4, 2, 1, 2];
                            case "FIR":
                            case "FIS":
                                return [4, 0]
                            }
                            return [4, 4]
                        }

                        startX: 1; startY: 1
                        PathLine { x: 1;           y: box.height-1 }
                        PathLine { x: box.width-1; y: box.height-1 }
                        PathLine { x: box.width-1; y: 1 }
                        PathLine { x: 1;           y: 1 }
                    }
                }

                Rectangle {
                    width: box.width
                    height: box.height

                    border.color: {
                        switch(gridLYO.airspace.CAT) {
                        case "A":
                        case "B":
                        case "C":
                        case "D":
                            return "#400000ff";
                        case "DNG":
                        case "P":
                        case "R":
                            return "#40ff0000";
                        case "ATZ":
                        case "RMZ":
                        case "TIZ":
                        case "TIA":
                            return "#400000ff";
                        case "NRA":
                            return "#4000ff00";
                        }
                        return "transparent"
                    }
                    border.width: 6

                    color: {
                        switch(gridLYO.airspace.CAT) {
                        case "CTR":
                            return "#40ff0000";
                        case "GLD":
                            return "#40ffff00";
                        case "ATZ":
                        case "RMZ":
                        case "TIZ":
                        case "TIA":
                            return "#400000ff";
                        }
                        return "transparent"
                    }

                    Label {
                        anchors.centerIn: parent
                        text: gridLYO.airspace.CAT
                    }

                }

            }

            Label {
                Layout.fillWidth: true
                Layout.rowSpan: 3
                Layout.alignment: Qt.AlignVCenter
                text: gridLYO.airspace.name
                wrapMode: Text.WordWrap
            }

            Label {
                id: colorGlean
                Layout.alignment: Qt.AlignHCenter|Qt.AlignBottom
                text: {
                    switch(Navigator.aircraft.verticalDistanceUnit) {
                    case Aircraft.Feet:
                        return gridLYO.airspace.upperBound
                    case Aircraft.Meters:
                        return gridLYO.airspace.upperBoundMetric
                    }
                }
                wrapMode: Text.WordWrap
            }

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                color:  colorGlean.color
                Layout.preferredHeight: 1
                Layout.preferredWidth: colorGlean.font.pixelSize*5
            }

            Label {
                Layout.alignment: Qt.AlignHCenter|Qt.AlignTop
                text: {
                    switch(Navigator.aircraft.verticalDistanceUnit) {
                    case Aircraft.Feet:
                        return gridLYO.airspace.lowerBound
                    case Aircraft.Meters:
                        return gridLYO.airspace.lowerBoundMetric
                    }
                }
                wrapMode: Text.WordWrap
            }
        }
    }

    Component {
        id: vacButtonDelegate

        RowLayout {
            id: vb

            Layout.preferredWidth: sv.width

            property vac vac

            Icon {
                Layout.preferredWidth: button.font.pixelSize*3
                Layout.alignment: Qt.AlignVCenter
                source: "/icons/material/ic_map.svg"
            }

            Button {
                id: button
                text: "<a href='xx'>" + vb.vac.name + "</a>"
                flat: true
                Layout.alignment: Qt.AlignVCenter
                onPressed:  {
                    PlatformAdaptor.vibrateBrief()
                    Global.currentVAC = vb.vac
                    waypointDescriptionDialog.close()
                }
            }

            Item {
                Layout.fillWidth: true
            }
        }
    }

    Component {
        id: satButtonDelegate

        RowLayout {
            Layout.preferredWidth: sv.width

            Icon {
                Layout.preferredWidth: button.font.pixelSize*3
                Layout.alignment: Qt.AlignVCenter
                source: "/icons/material/ic_open_in_browser.svg"
            }

            Button {
                id: button
                text: "<a href='xx'>" + qsTr("Satellite View") + "</a>"
                flat: true
                Layout.alignment: Qt.AlignVCenter
                onPressed:  {
                    PlatformAdaptor.vibrateBrief()
                    if (GlobalSettings.alwaysOpenExternalWebsites === true)
                    {
                        PlatformAdaptor.openSatView(waypointDescriptionDialog.waypoint.coordinate)
                        return
                    }
                    privacyWarning.coordinate = waypointDescriptionDialog.waypoint.coordinate
                    privacyWarning.open()
                }
            }

            Item {
                Layout.fillWidth: true
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent

        Label { // Second header line with distance and QUJ
            text: Navigator.aircraft.describeWay(PositionProvider.positionInfo.coordinate(), waypointDescriptionDialog.waypoint.coordinate)
            visible: (text !== "")
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignRight
            wrapMode: Text.WordWrap
        }

        DecoratedScrollView {
            id: sv

            Layout.fillWidth: true
            Layout.fillHeight: true

            contentHeight: co.height
            contentWidth: availableWidth // Disable horizontal scrolling

            clip: true

            ColumnLayout {
                id: co
                width: parent.width
            }

        } // DecoratedScrollView

        Keys.onBackPressed: (event) => {
            event.accepted = true;
            waypointDescriptionDialog.close()
        }
    }


    footer: DialogButtonBox {

        Button {
            text: qsTr("Route")
            flat: true

            onClicked: {
                PlatformAdaptor.vibrateBrief()
                addMenu.open()
            }

            AutoSizingMenu {
                id: addMenu

                Action {
                    text: qsTr("Direct")
                    enabled: PositionProvider.receivingPositionInfo && (dialogLoader.text !== "noRouteButton")

                    onTriggered: {
                        PlatformAdaptor.vibrateBrief()
                        if (Navigator.flightRoute.size > 0) {
                            addMenu.close()
                            overwriteDialog.open()
                        }
                        else {
                            Navigator.flightRoute.clear()
                            Navigator.flightRoute.append(PositionProvider.lastValidCoordinate)
                            Navigator.flightRoute.append(waypointDescriptionDialog.waypoint)
                            Global.toast.doToast(qsTr("New flight route: direct to %1.").arg(waypointDescriptionDialog.waypoint.extendedName))
                            addMenu.close()
                            waypointDescriptionDialog.close()
                        }
                    }
                }

                Rectangle {
                    height: 1
                    Layout.fillWidth: true
                    color: "black"
                }

                Action {
                    text: qsTr("Append")
                    enabled: {
                        // Mention Object to ensure that property gets updated
                        // when flight route changes
                        Navigator.flightRoute.size
                        return Navigator.flightRoute.canAppend(waypointDescriptionDialog.waypoint)
                    }

                    onTriggered: {
                        PlatformAdaptor.vibrateBrief()
                        Navigator.flightRoute.append(waypointDescriptionDialog.waypoint)
                        addMenu.close()
                        waypointDescriptionDialog.close()
                        Global.toast.doToast(qsTr("Added %1 to route.").arg(waypointDescriptionDialog.waypoint.extendedName))
                    }
                }

                Action {
                    text: qsTr("Insert")
                    enabled: {
                        // Mention Object to ensure that property gets updated
                        // when flight route changes
                        Navigator.flightRoute.size

                        return Navigator.flightRoute.canInsert(waypointDescriptionDialog.waypoint)
                    }

                    onTriggered: {
                        PlatformAdaptor.vibrateBrief()
                        Navigator.flightRoute.insert(waypointDescriptionDialog.waypoint)
                        addMenu.close()
                        waypointDescriptionDialog.close()
                        Global.toast.doToast(qsTr("Inserted %1 into route.").arg(waypointDescriptionDialog.waypoint.extendedName))
                    }
                }

                Action {
                    text: qsTr("Remove")

                    enabled:  {
                        // Mention to ensure that property gets updated
                        // when flight route changes
                        Navigator.flightRoute.size

                        return Navigator.flightRoute.contains(waypointDescriptionDialog.waypoint)
                    }
                    onTriggered: {
                        PlatformAdaptor.vibrateBrief()                        
                        var index = Navigator.flightRoute.lastIndexOf(waypointDescriptionDialog.waypoint)
                        if (index < 0)
                            return
                        Navigator.flightRoute.removeWaypoint(index)
                        addMenu.close()
                        waypointDescriptionDialog.close()
                        Global.toast.doToast(qsTr("Removed %1 from route.").arg(waypointDescriptionDialog.waypoint.extendedName))
                    }
                }
            }
        }

        Button {
            text: qsTr("Library")
            enabled: waypointDescriptionDialog.waypoint.category === "WP" //TODO: Warum kann ich keine nearby waypoints speichern?
            flat: true

            onClicked: {
                PlatformAdaptor.vibrateBrief()
                libraryMenu.open()
            }

            AutoSizingMenu {
                id: libraryMenu

                Action {
                    text: qsTr("Add…")
                    enabled: !WaypointLibrary.hasNearbyEntry(waypointDescriptionDialog.waypoint)

                    onTriggered: {
                        PlatformAdaptor.vibrateBrief()
                        wpAdd.waypoint = waypointDescriptionDialog.waypoint
                        wpAdd.open()
                        libraryMenu.close()
                    }
                }

                Action {
                    text: qsTr("Remove…")
                    enabled: WaypointLibrary.contains(waypointDescriptionDialog.waypoint)

                    onTriggered: {
                        PlatformAdaptor.vibrateBrief()
                        removeDialog.waypoint = waypointDescriptionDialog.waypoint
                        removeDialog.open()
                        libraryMenu.close()
                    }
                }                

                Rectangle {
                    height: 1
                    Layout.fillWidth: true
                    color: "black"
                }

                Action {
                    text: qsTr("Edit…")
                    enabled: WaypointLibrary.contains(waypointDescriptionDialog.waypoint)

                    onTriggered: {
                        PlatformAdaptor.vibrateBrief()
                        wpEdit.waypoint = waypointDescriptionDialog.waypoint
                        wpEdit.open()
                        libraryMenu.close()
                    }
                }

            }
        }

        onRejected: waypointDescriptionDialog.close()
    }


    CenteringDialog {
        id: privacyWarning

        property var coordinate

        modal: true

        title: qsTr("Privacy warning")

        ColumnLayout {
            anchors.fill: parent

            DecoratedScrollView{
                Layout.fillHeight: true
                Layout.fillWidth: true

                contentWidth: availableWidth // Disable horizontal scrolling

                clip: true

                Label {
                    id: lbl
                    text: "<p>"
                          + qsTr("In order to show a satellite view, <strong>Enroute Flight Navigation</strong> will ask your system to open Google Earth or Google Maps in an external web browser or a dedicated app.")
                          + " " + qsTr("The authors of <strong>Enroute Flight Navigation</strong> do not control Google Earth or Google Maps.")
                          + " " + qsTr("They do not know what data it collects or how that data is processed.")
                          + "</p>"
                          + "<p>"
                          + " " + qsTr("With the click on OK, you consent to opening Google Earth or Google Maps on your device.")
                          + " " + qsTr("Click OK only if you agree with the terms and privacy policies of that site.")
                          + "</p>"

                    width: privacyWarning.availableWidth
                    textFormat: Text.RichText
                    wrapMode: Text.Wrap
                }
            }

            Item {
                Layout.preferredHeight: lbl.font.pixelSize
            }

            WordWrappingCheckDelegate {
                id: alwaysOpen

                Layout.fillWidth: true

                text: qsTr("Always open external web sites and apps, do not ask again")
                checked: GlobalSettings.alwaysOpenExternalWebsites
            }
        }

        standardButtons: Dialog.Cancel|Dialog.Ok

        onAccepted: {
            PlatformAdaptor.vibrateBrief()
            GlobalSettings.alwaysOpenExternalWebsites = alwaysOpen.checked
            PlatformAdaptor.openSatView(coordinate)
        }
    }

    LongTextDialog {
        id: overwriteDialog

        title: qsTr("Overwrite Current Flight Route?")
        text: qsTr("Once overwritten, the current flight route cannot be restored.")

        standardButtons: Dialog.No | Dialog.Yes
        modal: true

        onAccepted: {
            PlatformAdaptor.vibrateBrief()
            Navigator.flightRoute.clear()
            Navigator.flightRoute.append(waypointDescriptionDialog.waypoint)
            overwriteDialog.close()
            waypointDescriptionDialog.close()
            Global.toast.doToast(qsTr("New flight route: direct to %1.").arg(waypointDescriptionDialog.waypoint.extendedName))
        }
        onRejected: {
            PlatformAdaptor.vibrateBrief()
            overwriteDialog.close()
            waypointDescriptionDialog.open()
        }
    }

    WaypointEditor {
        id: wpEdit

        onAccepted: {
            PlatformAdaptor.vibrateBrief()
            var newWP = waypointDescriptionDialog.waypoint.copy()
            newWP.name = newName
            newWP.notes = newNotes
            newWP.coordinate = QtPositioning.coordinate(newLatitude, newLongitude, newAltitudeMeter)
            WaypointLibrary.replace(waypointDescriptionDialog.waypoint, newWP)
            waypointDescriptionDialog.close()
            Global.toast.doToast(qsTr("Modified entry %1 in library.").arg(newWP.extendedName))
        }
    }

    WaypointEditor {
        id: wpAdd

        title: qsTr("Add Waypoint to Library")

        onAccepted: {
            PlatformAdaptor.vibrateBrief()
            var newWP = waypointDescriptionDialog.waypoint.copy()
            newWP.name = newName
            newWP.notes = newNotes
            newWP.coordinate = QtPositioning.coordinate(newLatitude, newLongitude, newAltitudeMeter)
            WaypointLibrary.add(newWP)
            waypointDescriptionDialog.close()
            Global.toast.doToast(qsTr("Added %1 to waypoint library.").arg(newWP.extendedName))
        }
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
            waypointDescriptionDialog.close()
            Global.toast.doToast(qsTr("Waypoint removed from device"))
        }
        onRejected: {
            PlatformAdaptor.vibrateBrief()
            removeDialog.close()
        }
    }

} // Dialog
