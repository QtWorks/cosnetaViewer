import QtQuick 2.5
import "../.."

// Image button
ToolBarRadioButton {
    id: root
    property variant itemData
    source: itemData.icon
    signal toolBarItemClicked(variant itemData)
    onButtonClicked: toolBarItemClicked(itemData)
}
