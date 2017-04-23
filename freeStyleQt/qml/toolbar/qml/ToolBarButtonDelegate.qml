import QtQuick 2.5
import "../.."

// Toolbar button delegate
ToolBarButton {
    id: root
    property variant modeData
    source: modeData.icon
    signal toolBarItemClicked(variant modeData)
    onButtonClicked: toolBarItemClicked(modeData)
}
