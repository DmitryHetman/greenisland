/****************************************************************************
 * This file is part of Green Island.
 *
 * Copyright (C) 2014 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
 *
 * Author(s):
 *    Pier Luigi Fiorini
 *
 * $BEGIN_LICENSE:GPL2+$
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * $END_LICENSE$
 ***************************************************************************/

import QtQuick 2.0
import ".."

Item {
    readonly property string name: _greenisland_output.name
    readonly property int number: _greenisland_output.number
    readonly property bool primary: _greenisland_output.primary
    property alias showInformation: outputInfo.visible

    readonly property alias workspacesView: workspacesLayer
    readonly property alias currentWorkspace: workspacesLayer.currentWorkspace
    property alias zoomEnabled: zoomArea.enabled

    property var layers: QtObject {
        readonly property alias background: backgroundLayer
        readonly property alias desktop: desktopLayer
        readonly property alias workspaces: workspacesLayer
        readonly property alias panels: panelsLayer
        readonly property alias notifications: notificationsLayer
        readonly property alias fullScreen: fullScreenLayer
        readonly property alias dialogs: dialogsLayer
        readonly property alias overlays: overlayLayer
        readonly property alias lock: lockLayer
        readonly property alias splash: splashLayer
    }

    id: root
    transform: Scale {
        id: screenScaler
        origin.x: zoomArea.x2
        origin.y: zoomArea.y2
        xScale: zoomArea.zoom2
        yScale: zoomArea.zoom2
    }

    /*
     * Output information panel
     */

    OutputInfo {
        id: outputInfo
        anchors {
            left: parent.left
            top: parent.top
        }
        number: root.number
        primary: root.primary
        visible: false
        z: 4000
    }

    /*
     * Screen zoom handler
     */

    ScreenZoom {
        id: zoomArea
        anchors.fill: parent
        scaler: screenScaler
        enabled: true
        z: 3000
    }

    /*
     * Special layers
     */

    // Splash is above everything but the cursor layer
    Rectangle {
        id: splashLayer
        anchors.fill: parent
        color: "black"
        z: 2000

        Behavior on opacity {
            NumberAnimation {
                easing.type: Easing.InOutQuad
                duration: 250
            }
        }
    }

    // Modal overlay for dialogs
    Rectangle {
        id: modalOverlay
        anchors.fill: parent
        color: "black"
        opacity: 0.0

        // Globally modal dialogs can cover applications and shell gadgets
        Item {
            id: dialogsLayer
            anchors.fill: parent
        }

        Behavior on opacity {
            NumberAnimation {
                easing.type: Easing.InOutQuad
                duration: 250
            }
        }
    }

    // Lock screen is above all windows to shield the session
    Item {
        id: lockLayer
        anchors.fill: parent
    }

    /*
     * Workspace
     */

    Item {
        id: userLayer
        anchors.fill: parent

        // Application and shell windows
        Item {
            id: sessionLayer
            anchors.fill: parent

            // Background is below everything
            Image {
                id: backgroundLayer
                anchors.fill: parent
                source: "../../images/wallpaper.png"
                fillMode: Image.Tile
            }

            // Desktop is only above to the background
            Item {
                id: desktopLayer
                anchors.fill: parent
            }

            // Workspaces
            WorkspacesLinearView {
                id: workspacesLayer
                anchors.fill: parent
            }

            // Panels are above application windows
            Item {
                id: panelsLayer
                anchors.fill: parent
            }

            // Notifications are above panels
            Item {
                id: notificationsLayer
                anchors.fill: parent
            }

            // Overlays can cover pretty much everything except the lock screen
            Item {
                id: overlayLayer
                anchors.fill: parent
            }
        }

        // Full screen windows can cover application windows and panels
        Item {
            id: fullScreenLayer
            anchors.fill: parent
            visible: false
        }

        // Hot corners
        HotCorners {
            id: hotCorners
            anchors.fill: parent
            rotation: 0
            onTopLeftTriggered: workspacesLayer.selectPrevious()
            onTopRightTriggered: workspacesLayer.selectNext()
            onBottomLeftTriggered: compositorRoot.toggleEffect("PresentWindowsGrid")
        }
    }

    /**
     * Brings up the desired layer, the other layers are
     * given a 0 z-index hence are shown based on their order
     * in the code.
     **/
    function setCurrentLayer(type) {
        var lowIndex = 0;
        var highIndex = 2000;

        switch (type) {
        case "splash":
            splashLayer.z = highIndex;
            modalOverlay.z = lowIndex;
            lockLayer.z = lowIndex;
            userLayer.z = lowIndex;
            break;
        case "modal":
            splashLayer.z = lowIndex;
            modalOverlay.z = highIndex;
            lockLayer.z = lowIndex;
            userLayer.z = lowIndex;
            break;
        case "lock":
            splashLayer.z = lowIndex;
            modalOverlay.z = lowIndex;
            lockLayer.z = highIndex;
            userLayer.z = lowIndex;
            break;
        case "user":
            splashLayer.z = lowIndex;
            modalOverlay.z = lowIndex;
            lockLayer.z = lowIndex;
            userLayer.z = highIndex;
            break;
        default:
            break;
        }
    }
}
