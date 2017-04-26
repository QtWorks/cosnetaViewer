import QtQuick 2.5
import "../.."
import "../../generic"

Rectangle {
    id: root
    color: "transparent"
    width: Theme.toolbarItemSize
    height: Theme.toolbarItemSize + (labelContainer.visible ? Theme.buttonLabelHeight : 0)
    border.width: 3
    border.color: "transparent"
    property string label: ""
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
