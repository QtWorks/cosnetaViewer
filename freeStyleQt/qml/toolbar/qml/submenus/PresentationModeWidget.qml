import QtQuick 2.5
import "../../.."

Rectangle {
    color: Theme.widgetBackgroundColor
    border.color: "black"
    anchors.fill: parent
    Column {
        anchors.fill: parent
        Item {
            width: parent.width
            height: parent.height/2
            Row {
                anchors.fill: parent
                Item {
                    width: parent.width/3
                    height: parent.height
                    ToolBarButton {
                        source: "qrc:/qml/toolbar/icons/previous_slide.svg"
                        anchors.centerIn: parent
                        label: qsTr("Prev Slide")
                    }
                }
                Item {
                    width: parent.width/3
                    height: parent.height
                    ToolBarButton {
                        source: "qrc:/qml/toolbar/icons/next_slide.svg"
                        anchors.centerIn: parent
                        label: qsTr("Next Slide")
                    }
                }
                Item {
                    width: parent.width/3
                    height: parent.height
                    ToolBarButton {
                        source: "qrc:/qml/toolbar/icons/laser_pointer.svg"
                        anchors.centerIn: parent
                        label: qsTr("Laser Pointer")
                    }
                }
            }
        }
        Item {
            width: parent.width
            height: parent.height/2
            Row {
                anchors.fill: parent
                Item {
                    width: parent.width/3
                    height: parent.height
                    ToolBarButton {
                        source: "qrc:/qml/toolbar/icons/start_presentation.svg"
                        anchors.centerIn: parent
                        label: qsTr("Start Pres")
                    }
                }
                Item {
                    width: parent.width/3
                    height: parent.height
                    ToolBarButton {
                        source: "qrc:/qml/toolbar/icons/stop_presentation.svg"
                        anchors.centerIn: parent
                        label: qsTr("Stop Pres")
                    }
                }
                Item {
                    width: parent.width/3
                    height: parent.height
                    ToolBarButton {
                        source: "qrc:/qml/toolbar/icons/start_presentation.svg"
                        anchors.centerIn: parent
                        label: qsTr("Start Pres2")
                    }
                }
            }
        }
    }
}

