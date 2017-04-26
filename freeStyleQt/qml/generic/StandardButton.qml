import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import ".."

Button {
    id: root
    height: Theme.standardWidgetHeight
    property bool bold: true

    style: ButtonStyle {
        background: Rectangle {
            implicitWidth: Theme.standardWidgetWidth
            implicitHeight: Theme.standardWidgetHeight
            border.width: control.activeFocus ? 2 : 1
            border.color: "#888"
            radius: 4
            gradient: Gradient {
                GradientStop { position: 0 ; color: control.pressed ? "#ccc" : "#eee" }
                GradientStop { position: 1 ; color: control.pressed ? "#aaa" : "#ccc" }
            }
        }
        label: Label {
            font.family: Theme.mainFontFamily
            font.pixelSize: Theme.mainFontSize
            color: Theme.mainFontColor
            text: control.text
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
