import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import "."
import ".."
import "../.."
import "../../toolbar/qml"

PageBase {
    id: root
    headerVisible: true
    footerVisible: true

    property int cellSize: 48
    property int nItems: 5

    // Main toolbar
    headerContents: MainToolBar {
        id: mainToolBar
        anchors.right: parent.right
        anchors.top: parent.top
    }

    // Page contents
    pageContents: Item {
        anchors.fill: parent
    }

    // Initialize
    function initialize() {

    }

    // Finalize
    function finalize() {

    }

    // Next page name
    function nextPageName() {

    }
}
