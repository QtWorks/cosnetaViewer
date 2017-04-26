import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import "."
import ".."

PageBase {
    headerVisible: true
    footerVisible: true
    pageContents: Rectangle {
        anchors.fill: parent
        color: "red"
        Rectangle {
            anchors.fill: parent
            anchors.margins: 32
        }
    }

    // Initialize
    function initialize() {

    }

    // Finalize
    function finalize() {

    }
}
