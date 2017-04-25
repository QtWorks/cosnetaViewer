import QtQuick 2.5

PageMgr {
    id: root

    // Get page object
    function getPageObject(name)
    {
        for (var i=0; i<gPageSettings.length; i++)
        {
            var pageObject = gPageSettings[i]
            if ((typeof pageObject !== "undefined") && (pageObject !== null))
            {
                if (pageObject.name === name)
                    return pageObject
            }
        }
        return null
    }

    // Create page
    function createPage(pageIdentifier)
    {
        // Get page description
        var pageObject = getPageObject(pageIdentifier)
        if (!pageObject) {
            console.log("CAN'T GET PAGE: " + pageIdentifier)
            return null
        }

        // Create a new page component
        var component = Qt.createComponent(pageObject.url)
        if (!component) {
            console.log("CAN'T CREATE: " + pageObject.url)
            return null
        }

        // Create page
        var page = component.createObject(root, {"pageObject": pageObject})
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
