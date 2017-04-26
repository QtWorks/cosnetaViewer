import QtQuick 2.5
import "../.."

Item {
    id: root
    anchors.fill: parent
    property alias state: leftWing.state
    clip: true
    signal toolBarItemClicked(int modeId)
    CustomToolBar {
        id: leftWing
        model: gToolBarSettings.modes
        x: rightWing.x
        Behavior on x {
            NumberAnimation {duration: Theme.leftWingAnimationDuration; easing.type: Easing.OutElastic}
        }
        onToolBarItemClicked: {
            if (itemData.modeId !== controller.roomManager.currentRoom.currentMode)
                root.toolBarItemClicked(itemData.modeId)
        }
        states: State {
            name: "opened"
            PropertyChanges {
                target: leftWing
                x: rightWing.x-leftWing.width
            }
        }
    }
}
