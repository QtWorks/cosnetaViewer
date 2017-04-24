import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import "."
import "../.."

PageBase {
    id: root
    headerVisible: false
    footerVisible: false

    property int cellSize: 48
    property int nItems: 5

    pageContents: Item {
        anchors.centerIn: parent
        width: root.width < root.height ? root.width : root.height
        height: width

        // Background
        Image {
            id: background
            source: "qrc:/background/cosnetameetingpurple.jpg"
            anchors.centerIn: parent
            width: Math.min(root.width, root.height)
            fillMode: Image.PreserveAspectFit
        }

        // Input background
        Rectangle {
            id: rect
            anchors.fill: parent
            border.width: 3
            border.color: "black"
            color: "gray"
            anchors.centerIn: parent
            opacity: .25
        }

        // Central item
        Item {
            id: centralItem
            anchors.centerIn: parent
            width: parent.width
            height: 1
        }

        // Title
        StandardText {
            id: titleLabel
            anchors.bottom: enterDeviceNameField.top
            anchors.bottomMargin: 2*Theme.pageVerticalSpacing
            anchors.horizontalCenter: parent.horizontalCenter
            text: Theme.selectFreeStyleSessionToJoin
            horizontalAlignment: Text.AlignHCenter
        }

        // Enter device name
        StandardTextField {
            id: enterDeviceNameField
            anchors.bottom: enterPasswordField.top
            anchors.bottomMargin: Theme.pageVerticalSpacing
            anchors.left: parent.left
            anchors.leftMargin: Theme.pageHorizontalPadding
            anchors.right: parent.right
            anchors.rightMargin: Theme.pageHorizontalPadding
            title: Theme.deviceName
        }

        // Enter password
        StandardTextField {
            id: enterPasswordField
            anchors.bottom: centralItem.top
            anchors.bottomMargin: Theme.pageVerticalSpacing
            anchors.left: parent.left
            anchors.leftMargin: Theme.pageHorizontalPadding
            anchors.right: parent.right
            anchors.rightMargin: Theme.pageHorizontalPadding
            title: Theme.password
        }

        // Manual remote data
        StandardComboBox {
            id: manualRemoteDataBtn
            anchors.top: centralItem.bottom
            anchors.topMargin: Theme.pageVerticalSpacing
            anchors.left: parent.left
            anchors.leftMargin: Theme.pageHorizontalPadding
            anchors.right: parent.right
            anchors.rightMargin: Theme.pageHorizontalPadding
            model: controller.serverManager.serverNames
            visible: controller.serverManager.serverCount > 0
        }

        // Search status
        StandardText {
            id: searchStatusLabel
            color: Theme.redColor
            text: Theme.searchingForSessions
            anchors.top: centralItem.bottom
            anchors.topMargin: Theme.pageVerticalSpacing
            anchors.left: parent.left
            anchors.leftMargin: Theme.pageHorizontalPadding
            visible: controller.serverManager.serverCount < 1
        }

        // Connect, Help & Quit
        Row {
            id: row
            anchors.left: parent.left
            anchors.leftMargin: Theme.pageHorizontalPadding
            anchors.right: parent.right
            anchors.rightMargin: Theme.pageHorizontalPadding
            anchors.top: searchStatusLabel.bottom
            anchors.topMargin: Theme.pageVerticalSpacing
            height: Theme.standardWidgetHeight
            property int divider: controller.serverManager.serverCount > 0 ? 3 : 3

            Item {
                width: parent.width/row.divider
                height: parent.height
                visible: controller.serverManager.serverCount > 0
                StandardButton {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.rightMargin: Theme.pageHorizontalPadding/2
                    height: parent.height
                    text: Theme.connect
                    onClicked: {
                        loadNextPage()
                        controller.serverManager.remoteConnect()
                    }
                }
            }

            Item {
                width: parent.width/row.divider
                height: parent.height
                StandardButton {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.rightMargin: Theme.pageHorizontalPadding/2
                    height: parent.height
                    text: Theme.help
                }
            }

            Item {
                width: parent.width/row.divider
                height: parent.height
                StandardButton {
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.pageHorizontalSpacing/2
                    anchors.right: parent.right
                    height: parent.height
                    text: Theme.quit
                }
            }
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
        return "main"
    }
}
