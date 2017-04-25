import QtQuick 2.5
import QtQuick.Layouts 1.3
import "../.."

Item {
    id: root
    width: Theme.toolbarItemSize*12
    height: Theme.toolbarItemSize

    // Current drawing mode
    property string currentDrawingMode: "freehand"

    // Display mode options?
    property bool optionsVisible: false

    // Submenu url
    property variant subMenuData: []

    // Return item data
    function getItemData(modeId) {
        for (var i=0; i<gToolBarSettings.modes.length; i++) {
            if (modeId === gToolBarSettings.modes[i].modeId)
                return gToolBarSettings.modes[i]
        }
        return null
    }

    // Set current mode
    function setCurrentMode(modeId) {
        controller.currentMode = modeId
        rightWing.setCurrentMode(modeId)
    }

    // Left wing
    LeftWing {
        id: leftWing
        anchors.fill: parent
        onToolBarItemClicked: {
            console.log("SETTING MODE TO ", modeId)
            root.setCurrentMode(modeId)
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
            if (itemData.name !== "grab") {
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
            var itemData = gToolBarSettings.modes[i]
            var optionsMenuIsDefined = (typeof itemData.menuUrl !== "undefined") && (itemData.menuUrl !== null)
            var currentModeHasOptions = optionsMenuIsDefined && (itemData.menuUrl.length > 0)
            if (currentModeHasOptions)
                subMenuData.push(itemData)
            menuDisplay.model = subMenuData
        }
    }
}
