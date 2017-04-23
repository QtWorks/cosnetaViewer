import QtQuick 2.5
import "."

Item {
    id: root
    width: Theme.toolbarItemSize
    height: Theme.toolbarItemSize
    property alias source: image.source
    property bool borderVisible: false
    signal buttonClicked()

    // Show background
    onSourceChanged: {
        if (borderVisible && (menuDisplay.state === "")) {
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

    Image {
        id: image
        smooth: true
        antialiasing: true
        sourceSize.width: root.width
        sourceSize.height: root.height
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
}
