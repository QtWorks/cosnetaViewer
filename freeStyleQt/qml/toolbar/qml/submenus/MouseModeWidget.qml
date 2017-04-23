import QtQuick 2.5
import "../../.."

Rectangle {
    color: "transparent"
    border.color: "black"
    anchors.fill: parent
    Row {
        anchors.fill: parent
        Item {
            width: parent.width/3
            height: parent.height
            ToolBarButton {
                source: "qrc:/qml/toolbar/icons/left_mouse_click.svg"
                anchors.centerIn: parent
            }
        }
        Item {
            width: parent.width/3
            height: parent.height
            ToolBarButton {
                source: "qrc:/qml/toolbar/icons/right_mouse_click.svg"
                anchors.centerIn: parent
            }
        }
        Item {
            width: parent.width/3
            height: parent.height
            ToolBarButton {
                source: "qrc:/qml/toolbar/icons/text_entry.svg"
                anchors.centerIn: parent
            }
        }
    }
}
