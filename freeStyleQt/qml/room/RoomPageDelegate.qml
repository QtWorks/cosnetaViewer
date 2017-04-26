import QtQuick 2.5
import ".."
import "../pages"
import "../toolbar/qml"

PageBase {
    headerVisible: true
    footerVisible: false

    // Main toolbar
    headerContents: MainToolBar {
        id: mainToolBar
        anchors.right: parent.right
        anchors.top: parent.top
        onHideSubMenu: menuDisplay.state = ""
        onToggleMenuDisplayState: {
            if (menuDisplay.state === "")
                menuDisplay.state = "on"
            else
                menuDisplay.state = ""
        }
    }

    // Menu display
    MenuDisplay {
        id: menuDisplay
        anchors.top: header.bottom
        anchors.topMargin: Theme.toolbarItemSpacing
        anchors.right: parent.right
    }

    // On completed, build menu display
    Component.onCompleted: {
        var subMenuData = []
        for (var i=0; i<gToolBarSettings.modes.length; i++) {
            var itemData = gToolBarSettings.modes[i]
            var optionsMenuIsDefined = (typeof itemData.menuUrl !== "undefined") && (itemData.menuUrl !== null)
            var currentModeHasOptions = optionsMenuIsDefined && (itemData.menuUrl.length > 0)
            if (currentModeHasOptions)
                subMenuData.push(itemData)
            menuDisplay.model = subMenuData
        }
    }
}
