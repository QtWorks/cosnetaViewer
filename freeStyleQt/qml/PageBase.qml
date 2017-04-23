import QtQuick 2.5

Item {
    id: root
    property alias headerVisible: pageHeader.visible
    property alias headerContents: pageHeader.children
    property alias footerVisible: pageFooter.visible
    property alias footerContents: pageFooter.children
    property alias pageContents: pageContents.children

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
        objectName: "toto"
        anchors.top: pageHeader.bottom// headerVisible ? pageHeader.bottom : parent.top
        anchors.bottom: pageFooter.top //footerVisible ? pageFooter.top : parent.bottom
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
