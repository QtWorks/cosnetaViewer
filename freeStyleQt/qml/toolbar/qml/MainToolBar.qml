import QtQuick 2.5
import QtQuick.Layouts 1.3
import "../.."

Item {
    id: root
    width: Theme.toolbarItemSize*12
    height: Theme.toolbarItemSize

    // Current mode
    property string currentMode: "viewMode"

    // Current drawing mode
    property string currentDrawingMode: "freehand"

    // Display mode options?
    property bool optionsVisible: false

    // Submenu url
    property variant subMenuData: []

    // Return mode data
    function getModeData(modeName) {
        for (var i=0; i<gToolBarSettings.modes.length; i++) {
            if (modeName === gToolBarSettings.modes[i].name)
                return gToolBarSettings.modes[i]
        }
        return null
    }

    // Set current mode
    function setCurrentMode(modeName) {
        currentMode = modeName
        rightWing.setCurrentMode(modeName)
    }

    // Left wing
    LeftWing {
        id: leftWing
        anchors.fill: parent
        onToolBarItemClicked: {
            console.log("SETTING MODE TO ", modeName)
            root.setCurrentMode(modeName)
        }
    }

    // Background
    Rectangle {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        color: Theme.toolbarBackgroundColor
        width: rightWing.width
        height: Theme.toolbarItemSize
    }

    // Right wing
    RightWing {
        id: rightWing
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        onOpenMenu: leftWing.state = "opened"
        onCloseMenu: leftWing.state = ""
        onHideSubMenu: menuDisplay.state = ""
        onToolBarItemClicked: {
            if (modeData.name !== "grab") {
                if (menuDisplay.state === "")
                    menuDisplay.state = "on"
                else
                    menuDisplay.state = ""
            }
        }
    }

    // Menu display
    MenuDisplay {
        id: menuDisplay
        anchors.top: parent.bottom
        anchors.topMargin: Theme.toolbarItemSpacing
        anchors.right: parent.right
    }

    Component.onCompleted: {
        for (var i=0; i<gToolBarSettings.modes.length; i++) {
            var modeData = gToolBarSettings.modes[i]
            var optionsMenuIsDefined = (typeof modeData.menuUrl !== "undefined") && (modeData.menuUrl !== null)
            var currentModeHasOptions = optionsMenuIsDefined && (modeData.menuUrl.length > 0)
            if (currentModeHasOptions)
                subMenuData.push(modeData)
            menuDisplay.model = subMenuData
        }
    }
}
