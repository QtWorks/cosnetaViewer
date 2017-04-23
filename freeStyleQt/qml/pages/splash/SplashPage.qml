import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import "."
import "../.."

PageBase {
    id: root
    headerVisible: false
    footerVisible: false
    pageContents: Item {
        id: pageContents
        anchors.fill: parent

        Image {
            anchors.fill: parent
            anchors.margins: 8
            fillMode: Image.PreserveAspectFit
            source: "qrc:/background/cosneta_logo.png"
        }
    }

    Timer {
        id: splashTimer
        interval: Theme.splashTimerDuration
        repeat: false
        onTriggered: loadNextPage()
    }

    // Initialize
    function initialize() {
        splashTimer.start()
    }

    // Finalize
    function finalize() {

    }

    // Next page name
    function nextPageName() {
        return "login"
    }
}
