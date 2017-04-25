import QtQuick 2.5
import "../.."

Flow {
    id: root
    property int nItems: 3
    width: nItems*Theme.toolbarItemSize+(nItems-1)*Theme.toolbarItemSpacing
    height: Theme.toolbarItemSize
    spacing: Theme.toolbarItemSpacing
    property bool currentModeHasOptions: false
    signal openMenu()
    signal closeMenu()
    signal hideSubMenu()
    signal toolBarItemClicked(variant itemData)

    function openLeftWing() {
        openCloseMenuBton.state = "opened"
        root.openMenu()
    }

    function closeLeftWing() {
        openCloseMenuBton.state = ""
        root.closeMenu()
    }

    // Set current mode
    function setCurrentMode(modeId) {
        // Retrieve item data
        var itemData = getItemData(modeId)

        // Found mode
        if ((typeof itemData !== "undefined") && (itemData !== null)) {
            // Set default item number
            nItems = 3

            // Check if item has a menu
            modeOptionsBtn.itemData = itemData

            // Does mode have an associated menu?
            var optionsMenuIsDefined = (typeof itemData.menuUrl !== "undefined") && (itemData.menuUrl !== null)
            currentModeHasOptions = optionsMenuIsDefined && (itemData.menuUrl.length > 0)
            modeOptionsBtn.visible = currentModeHasOptions
            if (currentModeHasOptions)
                nItems = 4
        }
    }

    // Open/close menu
    ToolBarButtonDelegate {
        id: openCloseMenuBton
        itemData: {"name": "openCloseMenu", "icon": "qrc:/qml/toolbar/icons/arrow_previous.svg"}
        onToolBarItemClicked: {
            if (state === "")
                openLeftWing()
            else
                closeLeftWing()
        }

        states: State {
            name: "opened"
            PropertyChanges {
                target: openCloseMenuBton
                itemData: {"name": "openCloseMenu", "icon": "qrc:/qml/toolbar/icons/arrow_next.svg"}
            }
        }
    }

    // Cosneta logo
    ToolBarButtonDelegate {
        id: cosnetaBtn
        itemData: {"name": "cosneta", "icon": "qrc:/qml/toolbar/icons/app_menu_closed.svg"}
        onToolBarItemClicked: {
            if (root.state === "") {
                nItems = 2
                root.state = "compact"
                closeLeftWing()
                hideSubMenu()
            }
            else {
                setCurrentMode(controller.currentMode)
                root.state = ""
                openLeftWing()
                hideSubMenu()
            }
        }
    }

    // Mode options
    ToolBarButtonDelegate {
        id: modeOptionsBtn
        itemData: {"name": "modeOptions", "icon": "qrc:/qml/toolbar/icons/question.svg"}
        onToolBarItemClicked: root.toolBarItemClicked(itemData)
        visible: currentModeHasOptions
        borderVisible: true
    }

    // Camera button
    ToolBarButtonDelegate {
        id: cameraBtn
        itemData: {"name": "grab", "icon": "qrc:/qml/toolbar/icons/snapshot.svg"}
        onToolBarItemClicked: root.toolBarItemClicked(itemData)
    }

    // Compact mode
    states: State {
        name: "compact"
        PropertyChanges {
            target: openCloseMenuBton
            visible: false
        }
        PropertyChanges {
            target: modeOptionsBtn
            visible: false
        }
        PropertyChanges {
            target: cosnetaBtn
            itemData: {"name": "cosneta", "icon": "qrc:/qml/toolbar/icons/app_menu_open.svg"}
        }
    }
}
