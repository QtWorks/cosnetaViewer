import QtQuick 2.5

PageBase {
    property variant roomObject
    headerVisible: true
    footerVisible: true

    footerContents: Item {
        anchors.fill: parent
        // Add room
        ToolBarButton {
            anchors.left: parent.right
            source: "qrc:/icons/remove.svg"
        }

        // Remove room
        ToolBarButton {
            anchors.right: parent.right
            source: "qrc:/icons/remove.svg"
        }
    }
}
