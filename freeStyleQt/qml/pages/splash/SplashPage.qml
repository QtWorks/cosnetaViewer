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

        StandardText {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 8
            font.pixelSize: 64
            text: "Welcome to Cosneta"
        }

        Image {
            anchors.fill: parent
            fillMode: Image.PreserveAspectFit
            source: "qrc:/background/freestyle_logo.png"
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
