import QtQuick 2.5
import QtQml.Models 2.2
import "../generic"

Item {
    id: root

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
                text: roomName
            }
        }
    }

    // Main list view
    ListView {
        anchors.fill: parent
        orientation: Qt.Horizontal
        interactive: false
        model: visualModel
        currentIndex: controller.roomManager.currentRoomIndex
    }
}
