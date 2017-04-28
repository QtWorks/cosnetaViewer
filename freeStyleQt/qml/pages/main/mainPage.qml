import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.4
import "../../pages"
import "../../generic"
import "../../room"
import "../../toolbar/qml"
import "../.."

PageBase {
    id: root
    headerVisible: false
    footerVisible: true

    // Page contents
    pageContents: RoomPageMgr {
        id: roomPageMgr
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
                visible: controller.roomManager.currentRoomIndex > 0
                onButtonClicked: {
                    controller.roomManager.currentRoomIndex--
                    roomPageMgr.positionViewAtIndex(controller.roomManager.currentRoomIndex)
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
                visible: controller.roomManager.currentRoomIndex < (controller.roomManager.count-1)
                onButtonClicked: {
                    controller.roomManager.currentRoomIndex++
                    roomPageMgr.positionViewAtIndex(controller.roomManager.currentRoomIndex)
                }
            }
        }
    }

    // Menu display
    MenuDisplay {
        id: menuDisplay
        anchors.top: header.bottom
        anchors.topMargin: Theme.toolbarItemSpacing
        anchors.right: parent.right
    }

    // Initialize
    function initialize() {

    }

    // Finalize
    function finalize() {

    }

    // On completed, build menu display
    Component.onCompleted: {
        var subMenuData = []
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
