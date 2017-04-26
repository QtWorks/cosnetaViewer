import QtQuick 2.5
import QtQuick.Layouts 1.3
import "../.."

Item {
    id: root
    width: Theme.toolbarItemSize*12
    height: Theme.toolbarItemSize

    // Display mode options?
    property bool optionsVisible: false

    // Hide sub menu
    signal hideSubMenu()

    // Toggle menu display state
    signal toggleMenuDisplayState()

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
        controller.roomManager.currentRoom.currentMode = modeId
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
        onHideSubMenu: root.hideSubMenu()
        onCameraBtnClicked: console.log("CAMERA BUTTON CLICKED")
        onModeOptionsButtonClicked: root.toggleMenuDisplayState()
    }
}
