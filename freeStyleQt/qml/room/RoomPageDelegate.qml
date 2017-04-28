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
}
