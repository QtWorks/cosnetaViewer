import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import ".."

Item {
    id: root
    property string title
    property int margin: Theme.standardItemMargin
    height: Theme.standardWidgetHeight

    // Label
    Item {
        id: labelContainer
        width: 1.5*Theme.standardWidgetWidth
        height: Theme.standardWidgetHeight

        // Label
        StandardText {
            id: label
            text: root.title
            visible: root.title.length > 0
            anchors.left: parent.left
            horizontalAlignment: Text.AlignLeft
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    // Text field container
    Item {
        id: textFieldContainer
        height: Theme.standardWidgetHeight
        anchors.left: labelContainer.right
        anchors.leftMargin: 8
        anchors.right: parent.right

        // Text field
        TextField {
            id: textField
            anchors.fill: parent
            font.family: Theme.mainFontFamily
            font.pixelSize: Theme.mainFontSize-8
            onTextChanged: root.textChanged(text)
            style: TextFieldStyle {
                textColor: Theme.mainFontColor
                font.family: Theme.mainFontFamily
                background: Rectangle {
                    radius: 2
                    implicitWidth: 100
                    implicitHeight: 24
                    border.color: "#333"
                    border.width: 1
                }
            }
        }
    }
}
