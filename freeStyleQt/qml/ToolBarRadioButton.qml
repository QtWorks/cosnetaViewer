import QtQuick 2.5
import "."

Rectangle {
    id: root
    color: "transparent"
    width: Theme.toolbarItemSize
    height: Theme.toolbarItemSize
    border.width: 3
    border.color: "transparent"

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
        Behavior on scale {
            NumberAnimation {duration: Theme.radioButtonScaleAnimationDuration}
        }
    }

    // Button active state
    states: State {
        name: "active"
        PropertyChanges {
            target: root
            border.color: Theme.buttonHighlightBorderColor
            color: Theme.buttonHighlightColor
        }
        PropertyChanges {
            target: image
            scale: .75
        }
    }
}
