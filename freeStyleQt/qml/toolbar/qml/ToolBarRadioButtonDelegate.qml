import QtQuick 2.5
import "../.."

// Image button
ToolBarRadioButton {
    id: root
    property variant modeData
    source: modeData.icon
    signal toolBarItemClicked(variant modeData)
    onButtonClicked: toolBarItemClicked(modeData)
}
