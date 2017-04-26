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
    signal modeOptionsButtonClicked(variant itemData)
    signal cameraBtnClicked()

    // Open left wing
    function openLeftWing() {
        openCloseMenuBton.state = "opened"
        root.openMenu()
    }

    // Close left wing
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
    ToolBarButton {
        id: openCloseMenuBton
        source: "qrc:/qml/toolbar/icons/arrow_previous.svg"
        onButtonClicked: {
            if (state === "")
                openLeftWing()
            else
                closeLeftWing()
        }

        states: State {
            name: "opened"
            PropertyChanges {
                target: openCloseMenuBton
                source: "qrc:/qml/toolbar/icons/arrow_next.svg"
            }
        }
    }

    // Cosneta logo
    ToolBarButton {
        id: cosnetaBtn
        source: "qrc:/qml/toolbar/icons/app_menu_closed.svg"
        onButtonClicked: {
            if (root.state === "") {
                nItems = 2
                root.state = "compact"
                closeLeftWing()
                hideSubMenu()
            }
            else
            {
                setCurrentMode(controller.roomManager.currentRoom.currentMode)
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
        onToolBarItemClicked: root.modeOptionsButtonClicked(itemData)
        visible: currentModeHasOptions
        borderVisible: true
    }

    // Camera button
    ToolBarButton {
        id: cameraBtn
        source: "qrc:/qml/toolbar/icons/snapshot.svg"
        onButtonClicked: cameraBtnClicked()
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
