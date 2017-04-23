import QtQuick 2.5
import "../.."

Rectangle {
    id: root
    width: model.length*Theme.toolbarItemSize+(model.length-1)*Theme.toolbarItemSpacing
    height: Theme.toolbarItemSize
    anchors.verticalCenter: parent.verticalCenter
    color: Theme.toolbarBackgroundColor
    property alias model: listView.model
    signal toolBarItemClicked(variant modeData)

    // Main list view
    ListView {
        id: listView
        anchors.fill: parent
        orientation: Qt.Horizontal
        spacing: Theme.toolbarItemSpacing
        interactive: false
        clip: true
        delegate: ToolBarRadioButtonDelegate {
            modeData: modelData
            onToolBarItemClicked: root.toolBarItemClicked(modeData)
            state: modeData.name === currentMode ? "active" : ""
        }
    }
}
