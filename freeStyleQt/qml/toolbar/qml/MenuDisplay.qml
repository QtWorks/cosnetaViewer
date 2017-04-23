import QtQuick 2.5
import "../.."

Item {
    id: root
    width: 5*Theme.toolbarItemSize + 6*Theme.toolbarItemSpacing
    height: 2*Theme.toolbarItemSize + 8*Theme.toolbarItemSpacing
    property alias model: repeater.model
    opacity: 0

    Repeater {
        id: repeater
        Loader {
            anchors.fill: parent
            source: modelData.menuUrl
            visible: modelData.name === currentMode
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
