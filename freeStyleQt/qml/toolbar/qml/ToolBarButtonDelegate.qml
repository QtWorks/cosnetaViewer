import QtQuick 2.5
import "../.."

// Toolbar button delegate
ToolBarButton {
    id: root
    property variant itemData
    source: itemData.icon
    signal toolBarItemClicked(variant itemData)
    onButtonClicked: toolBarItemClicked(itemData)
}
