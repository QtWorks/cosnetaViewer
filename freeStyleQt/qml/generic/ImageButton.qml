import QtQuick 2.5
import ".."

Item {
    id: imgButton
    width: Theme.dynamicToolBarButtonSize
    height: Theme.dynamicToolBarButtonSize
    property alias source: img.source
    signal buttonClicked()

    Image {
        id: img
        sourceSize.width: 64
        sourceSize.height: 64
        smooth: true
        antialiasing: true

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            onClicked: buttonClicked()
        }
        states: State {
            name: "pressed"
            when: mouseArea.pressed
            PropertyChanges {
                target: img
                scale: .95
            }
        }
    }
}
