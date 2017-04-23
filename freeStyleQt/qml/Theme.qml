pragma Singleton
import QtQuick 2.5

Item {
    // Strings
    readonly property string selectFreeStyleSessionToJoin: qsTr("Select FreeStyle session to join")
    readonly property string deviceName: qsTr("Device Name")
    readonly property string password: qsTr("Password")
    readonly property string manualRemoteData: qsTr("Manual Remote Data")
    readonly property string searchingForSessions: qsTr("Searching For Sessions...")
    readonly property string connect: qsTr("Connect")
    readonly property string help: qsTr("Help")
    readonly property string quit: qsTr("Quit")

    // Fonts
    readonly property string mainFontFamily: "Segoe UI"
    readonly property int mainFontSize: 32
    readonly property color mainFontColor: "#4D4D4D"

    // Sizes
    readonly property int standardWidgetWidth: 128
    readonly property int standardWidgetHeight: 48
    readonly property int pageHorizontalSpacing: 16
    readonly property int pageVerticalSpacing: 16
    readonly property int pageHorizontalPadding: 16

    // General colors
    readonly property color redColor: "red"

    // Header
    readonly property int headerHeight: 48
    readonly property color headerColor: "#4D4D4D"

    // Footer
    readonly property int footerHeight: 48
    readonly property color footerColor: "#4D4D4D"

    // Splash page
    readonly property int splashTimerDuration: 2000

    // Dynamic toolbar
    readonly property int toolbarItemSpacing: 8
    readonly property int toolbarItemSize: 48
    readonly property color toolbarBackgroundColor: "lightgray"
    readonly property int leftWingAnimationDuration: 1500
    readonly property color buttonHighlightBorderColor: "orange"
    readonly property color buttonHighlightColor: "lightgray"
    readonly property string publicAnnotationTab1Title: qsTr("Pen")
    readonly property string publicAnnotationTab2Title: qsTr("Undo")
    readonly property string publicAnnotationTab3Title: qsTr("Page")
    readonly property string publicAnnotationTab4Title: qsTr("Save")
    readonly property int menuDisplayOpacityAnimationDuration: 250
    readonly property int radioButtonScaleAnimationDuration: 250
    readonly property int standardTabHeight: toolbarItemSize*3/4
}
