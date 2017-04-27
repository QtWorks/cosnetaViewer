import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.4
import "../../pages"
import "../../generic"
import "../../room"
import "../../toolbar/qml"

PageBase {
    id: root
    headerVisible: false
    footerVisible: true

    // Page contents
    pageContents: RoomPageMgr {
        anchors.fill: parent
    }

    // Footer
    footerContents: Row {
        anchors.fill: parent

        Item {
            width: parent.width/4
            height: parent.height

            // Previous room
            ToolBarButton {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                source: "qrc:/icons/previous_page.svg"
                onButtonClicked: {
                    controller.roomManager.currentRoomIndex--
                    console.log("ICI: ", controller.roomManager.currentRoomIndex)
                }
            }
        }

        Item {
            width: parent.width/4
            height: parent.height

            // Add room
            ToolBarButton {
                anchors.centerIn: parent
                onButtonClicked: controller.roomManager.addRoom()
            }
        }

        Item {
            width: parent.width/4
            height: parent.height

            // Remove current room
            ToolBarButton {
                anchors.centerIn: parent
                onButtonClicked: controller.roomManager.removeCurrentRoom()
            }
        }

        Item {
            width: parent.width/4
            height: parent.height

            // Next room
            ToolBarButton {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                source: "qrc:/icons/next_page.svg"
                onButtonClicked: {
                    controller.roomManager.currentRoomIndex++
                    console.log("ICI: ", controller.roomManager.currentRoomIndex)
                }
            }
        }
    }

    // Initialize
    function initialize() {

    }

    // Finalize
    function finalize() {

    }
}
