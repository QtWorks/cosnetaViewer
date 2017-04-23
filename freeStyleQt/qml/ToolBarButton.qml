import QtQuick 2.5
import "."

Item {
    id: root
    width: Theme.toolbarItemSize
    height: Theme.toolbarItemSize
    property alias source: image.source
    signal buttonClicked()

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
}
