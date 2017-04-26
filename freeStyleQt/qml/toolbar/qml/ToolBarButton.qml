import QtQuick 2.5
import "../.."
import "../../generic"

Item {
    id: root
    width: Theme.toolbarItemSize
    height: Theme.toolbarItemSize + (labelContainer.visible ? Theme.buttonLabelHeight : 0)
    property alias source: image.source
    property bool borderVisible: false
    property string label: ""
    signal buttonClicked()

    // Show background
    onSourceChanged: {
        if (borderVisible) {
            borderAnim.stop()
            borderAnim.start()
        }
    }

    // Background
    Rectangle {
        anchors.fill: parent
        color: "orange"
        visible: borderVisible
        SequentialAnimation on opacity {
            id: borderAnim
            loops: 3
            NumberAnimation { from: 0; to: 1; duration: 500}
            NumberAnimation { from: 1; to: 0; duration: 500}
        }
    }

    // Image
    Image {
        id: image
        smooth: true
        antialiasing: true
        sourceSize.width: Theme.toolbarItemSize
        sourceSize.height: Theme.toolbarItemSize
        MouseArea {
            id: mouseArea
            anchors.fill: parent
            onClicked: buttonClicked()
        }
        states: State {
            name: "pressed"
            when: mouseArea.pressed
            PropertyChanges {
                target: image
                scale: .9
            }
        }
    }

    // Label area
    Item {
        id: labelContainer
        anchors.bottom: parent.bottom
        width: parent.width
        height: Theme.buttonLabelHeight
        visible: root.label.length > 0

        // Label
        StandardText {
            id: label
            height: parent.height
            anchors.centerIn: parent
            font.pixelSize: Theme.buttonLabelPixelSize
            text: root.label
            color: "black"
        }
    }
}
