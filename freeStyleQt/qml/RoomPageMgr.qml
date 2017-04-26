import QtQuick 2.5

PageMgr {
    // Create page
    function createPage(pageIdentifier)
    {
        // Create a new page component
        var component = Qt.createComponent(pageIdentifier)
        if (!component) {
            console.log("CAN'T CREATE: " + pageIdentifier)
            return null
        }

        // Create page
        var page = component.createObject(root)
        if (!page)
        {
            console.log("CREATEPAGE ERROR: " + component.errorString())
            return null
        }

        // Initialize page
        page.initialize()

        return page
    }
}
