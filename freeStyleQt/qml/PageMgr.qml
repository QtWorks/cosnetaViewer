import QtQuick 2.5
import QtQuick.Controls 1.2
import QtQuick.Window 2.0

StackView {
    id: pageMgr

    // Depth changed
    onDepthChanged: console.log("--- DEPTH --- " + depth)

    // Current page id
    property string currentPageName: ""
    onCurrentItemChanged: {
        if (currentItem)
            currentPageName = currentItem.pageObject.name
    }

    // Public interface
    signal loadFirstPage()
    signal loadNextPage()
    signal loadPreviousPage()
    signal loadPage(string pageName)

    // Initialize
    function initialize()
    {
        console.log("ICI: ", gPageSettings)
        if (gPageSettings.length > 0)
            loadPage(gPageSettings[0].name)
    }

    // Create page
    function createPage(pageName)
    {
        // Get page description
        var pageObject = getPageObject(pageName)
        if (!pageObject) {
            console.log("CAN'T GET PAGE: " + pageName)
            return null
        }

        // Create a new page component
        var component = Qt.createComponent(pageObject.url)
        if (!component) {
            console.log("CAN'T CREATE: " + pageObject.url)
            return null
        }

        // Create page
        var page = component.createObject(pageMgr, {"pageObject": pageObject})
        if (!page)
        {
            console.log("CREATEPAGE ERROR: " + component.errorString())
            return null
        }

        // Initialize page
        page.initialize()

        return page
    }

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

    // Load next page
    function onLoadNextPage()
    {
        if ((typeof currentItem !== "undefined") && (currentItem !== null))
        {
            // Finalize
            currentItem.finalize()

            var nextPage = createPage(currentItem.nextPageName())
            if ((typeof nextPage !== "undefined") && (nextPage !== null))
                push(nextPage)
        }
    }

    // Load previous page
    function onLoadPreviousPage()
    {
        // Finalize current item
        if ((typeof currentItem !== "undefined") && (currentItem !== null))
            currentItem.finalize()

        // Pop
        pop()

        // Initialize new item
        currentItem.initialize()
    }

    // Load first page
    function onLoadFirstPage()
    {
        // Finalize current item
        if ((typeof currentItem !== "undefined") && (currentItem !== null))
            currentItem.finalize()

        // Pop first
        pop(get(0))

        // Initialize current item
        if ((typeof currentItem !== "undefined") && (currentItem !== null))
            currentItem.initialize()
    }

    // Load specific page
    function onLoadPage(pageName)
    {
        // Finalize curret item
        if ((typeof currentItem !== "undefined") && (currentItem !== null))
            currentItem.finalize()

        var pageFound = false
        for (var i=0; i<depth; i++) {
            if (get(i, true).pageObject.name === pageName) {
                pop(get(i))
                pageFound = true
                break
            }
        }

        if (pageFound === false) {
            var page = createPage(pageName)
            if ((typeof page !== "undefined") && (page !== null))
                push(page)
        }
    }

    Component.onCompleted: {
        loadFirstPage.connect(onLoadFirstPage)
        loadPreviousPage.connect(onLoadPreviousPage)
        loadNextPage.connect(onLoadNextPage)
        loadPage.connect(onLoadPage)
        initialize()
    }
}
