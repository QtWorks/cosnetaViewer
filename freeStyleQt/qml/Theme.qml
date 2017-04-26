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
    readonly property color mainFontColor: "#8F5690"

    // Sizes
    readonly property int standardWidgetWidth: 128
    readonly property int standardWidgetHeight: 48
    readonly property int pageHorizontalSpacing: 16
    readonly property int pageVerticalSpacing: 16
    readonly property int pageHorizontalPadding: 16

    // General colors
    readonly property color redColor: "red"

    // Header
    readonly property int headerHeight: 52
    readonly property color headerColor: "#AAAAAA"

    // Footer
    readonly property int footerHeight: 52
    readonly property color footerColor: "#AAAAAA"

    // Splash page
    readonly property int splashTimerDuration: 2000

    // Dynamic toolbar
    readonly property int toolbarItemSpacing: 8
    readonly property int toolbarItemSize: 52
    readonly property color toolbarBackgroundColor: headerColor
    readonly property int leftWingAnimationDuration: 1500
    readonly property color buttonHighlightBorderColor: "orange"
    readonly property color buttonHighlightColor: "#AAAAAA"
    readonly property string publicAnnotationTab1Title: qsTr("Pen")
    readonly property string publicAnnotationTab2Title: qsTr("Undo")
    readonly property string publicAnnotationTab3Title: qsTr("Page")
    readonly property string publicAnnotationTab4Title: qsTr("Save")
    readonly property int menuDisplayOpacityAnimationDuration: 250
    readonly property int radioButtonScaleAnimationDuration: 250
    readonly property int standardTabHeight: toolbarItemSize*3/4

    // Widget background color
    readonly property color widgetBackgroundColor: headerColor

    // Button label height
    readonly property int buttonLabelHeight: 16
    readonly property int buttonLabelPixelSize: 12

    // Standard item spacing
    readonly property int standardItemSpacing: 8

    // Standard item margin
    readonly property int standardItemMargin: 8

    // Menu display
    readonly property int menuDisplayColumnNumber: 5
    readonly property int menuDisplayRowNumber: 2
}
