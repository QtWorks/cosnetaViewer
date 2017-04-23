import QtQuick 2.5
import "../.."

Item {
    id: root
    anchors.fill: parent
    property alias state: leftWing.state
    clip: true
    signal toolBarItemClicked(string modeName)
    CustomToolBar {
        id: leftWing
        model: gToolBarSettings.modes
        x: rightWing.x
        Behavior on x {
            NumberAnimation {duration: Theme.leftWingAnimationDuration; easing.type: Easing.OutElastic}
        }
        onToolBarItemClicked: {
            if (modeData.name !== currentMode)
                root.toolBarItemClicked(modeData.name)
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
