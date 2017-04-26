import QtQuick 2.5
import "../.."

Item {
    id: root
    property int nCols: 5
    property int nRows: 2
    width: nCols*Theme.toolbarItemSize + (nCols+1)*Theme.toolbarItemSpacing
    height: Theme.standardTabHeight + nRows*Theme.toolbarItemSize + nRows*Theme.buttonLabelHeight + nRows*Theme.toolbarItemSpacing
    property alias model: repeater.model
    opacity: 0

    Repeater {
        id: repeater
        Loader {
            anchors.fill: parent
            source: modelData.menuUrl
            visible: modelData.modeId === controller.roomManager.currentRoom.currentMode
        }
    }

    Behavior on opacity {
        NumberAnimation {duration: Theme.menuDisplayOpacityAnimationDuration}
    }

    states: State {
        name: "on"
        PropertyChanges {
            target: root
            opacity: 1
        }
    }
}
