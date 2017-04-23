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

    pageContents: Item {
        anchors.fill: parent
        MainToolBar {
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.top: parent.top
            anchors.topMargin: 8
        }
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
