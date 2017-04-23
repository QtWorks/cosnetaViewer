import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import "."
import ".."
import "../.."
import "../../toolbar/qml"

PageBase {
    id: root
    headerVisible: false
    footerVisible: false

    property int cellSize: 48
    property int nItems: 5

    // Page contents
    pageContents: Item {
        anchors.fill: parent

        // Background
        Rectangle {
            id: toolBarBackground
            anchors.left: parent.left
            anchors.right: parent.right
            height: mainToolBar.height
            color: Theme.toolbarBackgroundColor
        }

        // Main toolbar
        MainToolBar {
            id: mainToolBar
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.top: parent.top
        }

        /*
        // Back button
        ToolBarButton {
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.top: parent.top
            anchors.topMargin: 8
            source: "qrc:/icons/previous_page.svg"
            onButtonClicked: loadPreviousPage()
        }
        */
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
