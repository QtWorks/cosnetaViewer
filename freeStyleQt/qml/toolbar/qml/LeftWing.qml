import QtQuick 2.5
import "../.."

Item {
    id: root
    width: model.length*Theme.toolbarItemSize+(model.length-1)*Theme.toolbarItemSpacing
    height: Theme.toolbarItemSize
    anchors.verticalCenter: parent.verticalCenter
    property alias model: listView.model
    signal toolBarItemClicked(variant itemData)

    // Main list view
    ListView {
        id: listView
        anchors.fill: parent
        orientation: Qt.Horizontal
        spacing: Theme.toolbarItemSpacing
        interactive: false
        clip: true
        delegate: ToolBarRadioButtonDelegate {
            itemData: modelData
            onToolBarItemClicked: root.toolBarItemClicked(itemData)
            state: itemData.modeId === controller.roomManager.currentRoom.currentMode ? "active" : ""
        }
    }

    // Define behavior on x
    Behavior on x {
        NumberAnimation {duration: Theme.leftWingAnimationDuration; easing.type: Easing.OutElastic}
    }

    // Define states
    states: State {
        name: "opened"
        PropertyChanges {
            target: leftWing
            x: rightWing.x-leftWing.width
        }
    }
}
