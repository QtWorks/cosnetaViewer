import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.4
import "../../.."
import ".."

TabView {
    id: root
    anchors.fill: parent

    style: TabViewStyle {
        tabOverlap: 0
        tab: Rectangle {
            color: styleData.selected ? "orange" : "#8F5690"
            border.color:  "steelblue"
            implicitWidth: root.width/root.count
            implicitHeight: Theme.standardTabHeight

            // Tab label
            Text {
                anchors.centerIn: parent
                text: styleData.title
                font.family: Theme.mainFontFamily
                color: styleData.selected ? "white" : "black"
                font.pixelSize: Theme.toolbarItemSize/2.25
            }
        }
        frame: Rectangle { color: "steelblue" }
    }

    // Public annotation tab
    Tab {
        title: Theme.publicAnnotationTab1Title
        Rectangle {
            color: Theme.widgetBackgroundColor
            border.color: "black"
            Column {
                anchors.fill: parent
                Row {
                    width: parent.width
                    height: parent.height/2
                    Item {
                        width: parent.width/3
                        height: parent.height
                        ToolBarButton {
                            source: "qrc:/qml/toolbar/icons/colour_palette.svg"
                            label: qsTr("Colour")
                            anchors.centerIn: parent
                        }
                    }
                    Item {
                        width: parent.width/3
                        height: parent.height
                        ToolBarButton {
                            source: "qrc:/qml/toolbar/icons/thickness.svg"
                            label: qsTr("Thickness")
                            anchors.centerIn: parent
                        }
                    }
                    Item {
                        width: parent.width/3
                        height: parent.height
                        ToolBarButton {
                            source: "qrc:/qml/toolbar/icons/text_entry.svg"
                            label: qsTr("Send Text")
                            anchors.centerIn: parent
                        }
                    }
                }
                Row {
                    width: parent.width
                    height: parent.height/2
                    Item {
                        width: parent.width/5
                        height: parent.height
                        ToolBarRadioButton {
                            source: "qrc:/qml/toolbar/icons/freehand.svg"
                            anchors.centerIn: parent
                            onButtonClicked: currentDrawingMode = "freehand"
                            state: currentDrawingMode === "freehand" ? "active" : ""
                            label: qsTr("Line")
                        }
                    }
                    Item {
                        width: parent.width/5
                        height: parent.height
                        ToolBarRadioButton {
                            source: "qrc:/qml/toolbar/icons/triangle_stamp.svg"
                            anchors.centerIn: parent
                            onButtonClicked: currentDrawingMode = "triangle"
                            state: currentDrawingMode === "triangle" ? "active" : ""
                            label: qsTr("Tri")
                        }
                    }
                    Item {
                        width: parent.width/5
                        height: parent.height
                        ToolBarRadioButton {
                            source: "qrc:/qml/toolbar/icons/square_stamp.svg"
                            anchors.centerIn: parent
                            onButtonClicked: currentDrawingMode = "square"
                            state: currentDrawingMode === "square" ? "active" : ""
                            label: qsTr("Squ")
                        }
                    }
                    Item {
                        width: parent.width/5
                        height: parent.height
                        ToolBarRadioButton {
                            source: "qrc:/qml/toolbar/icons/circle_stamp.svg"
                            anchors.centerIn: parent
                            onButtonClicked: currentDrawingMode = "circle"
                            state: currentDrawingMode === "circle" ? "active" : ""
                            label: qsTr("Circle")
                        }
                    }
                    Item {
                        width: parent.width/5
                        height: parent.height
                        ToolBarRadioButton {
                            source: "qrc:/qml/toolbar/icons/clone_stamp.svg"
                            anchors.centerIn: parent
                            onButtonClicked: currentDrawingMode = "stamp"
                            state: currentDrawingMode === "stamp" ? "active" : ""
                            label: qsTr("Stamp")
                        }
                    }
                }
            }
        }
    }
    Tab {
        title: Theme.publicAnnotationTab2Title
        Rectangle {
            color: Theme.widgetBackgroundColor
            border.color: "black"
            Row {
                width: parent.width
                height: parent.height
                Item {
                    width: parent.width/3
                    height: parent.height
                    ToolBarButton {
                        source: "qrc:/qml/toolbar/icons/undo.svg"
                        anchors.centerIn: parent
                        label: qsTr("Undo")
                    }
                }
                Item {
                    width: parent.width/3
                    height: parent.height
                    ToolBarButton {
                        source: "qrc:/qml/toolbar/icons/redo.svg"
                        anchors.centerIn: parent
                        label: qsTr("Redo")
                    }
                }
                Item {
                    width: parent.width/3
                    height: parent.height
                    ToolBarButton {
                        source: "qrc:/qml/toolbar/icons/undo_all.svg"
                        anchors.centerIn: parent
                        label: qsTr("Redo All")
                    }
                }
            }
        }
    }
    Tab {
        title: Theme.publicAnnotationTab3Title
        Rectangle {
            color: Theme.widgetBackgroundColor
            border.color: "black"
            Column {
                anchors.fill: parent
                Row {
                    width: parent.width
                    height: parent.height/2
                    Item {
                        width: parent.width/2
                        height: parent.height
                        ToolBarButton {
                            source: "qrc:/qml/toolbar/icons/colour_palette.svg"
                            anchors.centerIn: parent
                            label: qsTr("Fill BG")
                        }
                    }
                    Item {
                        width: parent.width/2
                        height: parent.height
                        ToolBarButton {
                            source: "qrc:/qml/toolbar/icons/redo.svg"
                            anchors.centerIn: parent
                            label: qsTr("Clear BG")
                        }
                    }
                }
                Row {
                    width: parent.width
                    height: parent.height/2
                    Item {
                        width: parent.width/4
                        height: parent.height
                        ToolBarButton {
                            source: "qrc:/qml/toolbar/icons/display_thumbnails.svg"
                            anchors.centerIn: parent
                            label: qsTr("New Page")
                        }
                    }
                    Item {
                        width: parent.width/4
                        height: parent.height
                        ToolBarButton {
                            source: "qrc:/qml/toolbar/icons/previous_page.svg"
                            anchors.centerIn: parent
                            label: qsTr("Next")
                        }
                    }
                    Item {
                        width: parent.width/4
                        height: parent.height
                        ToolBarButton {
                            source: "qrc:/qml/toolbar/icons/next_page.svg"
                            anchors.centerIn: parent
                            label: qsTr("Last")
                        }
                    }
                    Item {
                        width: parent.width/4
                        height: parent.height
                        ToolBarButton {
                            source: "qrc:/qml/toolbar/icons/redo.svg"
                            anchors.centerIn: parent
                            label: qsTr("Gallery")
                        }
                    }
                }
            }
        }
    }
    Tab {
        title: Theme.publicAnnotationTab4Title
        Rectangle {
            color: Theme.widgetBackgroundColor
            border.color: "black"
            Row {
                width: parent.width
                height: parent.height
                Item {
                    width: parent.width/5
                    height: parent.height
                    ToolBarButton {
                        source: "qrc:/qml/toolbar/icons/record.svg"
                        anchors.centerIn: parent
                        label: qsTr("Record")
                    }
                }
                Item {
                    width: parent.width/5
                    height: parent.height
                    ToolBarButton {
                        source: "qrc:/qml/toolbar/icons/play_video.svg"
                        anchors.centerIn: parent
                        label: qsTr("Playback")
                    }
                }
                Item {
                    width: parent.width/5
                    height: parent.height
                    ToolBarButton {
                        source: "qrc:/qml/toolbar/icons/snapshot.svg"
                        anchors.centerIn: parent
                        label: qsTr("Snapshot")
                    }
                }
                Item {
                    width: parent.width/5
                    height: parent.height
                    ToolBarButton {
                        source: "qrc:/qml/toolbar/icons/display_thumbnails.svg"
                        anchors.centerIn: parent
                        label: qsTr("Gallery")
                    }
                }
                Item {
                    width: parent.width/5
                    height: parent.height
                    ToolBarButton {
                        source: "qrc:/qml/toolbar/icons/annotate_mode.svg"
                        anchors.centerIn: parent
                        label: qsTr("Save")
                    }
                }
            }
        }
    }
}
