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
    footerContents: RowLayout {
        anchors.fill: parent

        // Previous room
        ToolBarButton {
            anchors.verticalCenter: parent.verticalCenter
        }

        // Add room
        ToolBarButton {
            anchors.verticalCenter: parent.verticalCenter
            onButtonClicked: controller.roomManager.addRoom()
        }

        // Remove current room
        ToolBarButton {
            anchors.verticalCenter: parent.verticalCenter
            onButtonClicked: controller.roomManager.removeCurrentRoom()
        }

        // Next room
        ToolBarButton {
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    // Initialize
    function initialize() {

    }

    // Finalize
    function finalize() {

    }
}
