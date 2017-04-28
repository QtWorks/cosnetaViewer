import QtQuick 2.5
import QtQml.Models 2.2
import "../generic"

Item {
    id: root

    // Position view at index
    function positionViewAtIndex(index) {
        roomListView.positionViewAtIndex(index, ListView.SnapPosition)
    }

    // Delegate model
    DelegateModel {
        id: visualModel
        model: controller.roomManager
        delegate: RoomPageDelegate {
            id: roomPageDelegate
            width: root.width
            height: root.height

            StandardText {
                anchors.centerIn: parent
                color: "red"
                font.pixelSize: 48
                text: roomObject.name
            }
        }
    }

    // Main list view
    ListView {
        id: roomListView
        anchors.fill: parent
        orientation: Qt.Horizontal
        interactive: false
        model: visualModel
        snapMode: ListView.SnapOneItem
        highlightRangeMode: ListView.StrictlyEnforceRange
    }
}
