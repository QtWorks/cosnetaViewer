import QtQuick 2.5

Rectangle {
    id: root
    property alias header: pageHeader
    property alias headerVisible: pageHeader.visible
    property alias headerContents: pageHeader.children
    property alias footer: pageFooter
    property alias footerVisible: pageFooter.visible
    property alias footerContents: pageFooter.children
    property alias pageContents: pageContents.children
    color: "black"

    // Page object (from JSON)
    property variant pageObject

    // Page header
    PageHeader {
        id: pageHeader
        anchors.top: parent.top
        width: parent.width
    }

    // Page contents
    Item {
        id: pageContents
        anchors.top: headerVisible ? pageHeader.bottom : parent.top
        anchors.bottom: footerVisible ? pageFooter.top : parent.bottom
        width: parent.width
    }

    // Page footer
    PageFooter {
        id: pageFooter
        anchors.bottom: parent.bottom
        width: parent.width
    }

    // Initialize page (virtual)
    function initialize()
    {
    }

    // Finalize page (virtual)
    function finalize()
    {
    }

    // Return next page name (virtual)
    function nextPageName()
    {
        return ""
    }
}
