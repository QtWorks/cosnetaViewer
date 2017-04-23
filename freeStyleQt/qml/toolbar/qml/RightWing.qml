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
    signal toolBarItemClicked(variant modeData)

    function openLeftWing() {
        openCloseMenuBton.state = "opened"
        root.openMenu()
    }

    function closeLeftWing() {
        openCloseMenuBton.state = ""
        root.closeMenu()
    }

    // Set current mode
    function setCurrentMode(modeName) {
        // Retrieve mode data
        var modeData = getModeData(modeName)

        // Found mode
        if ((typeof modeData !== "undefined") && (modeData !== null)) {
            // Set default item number
            nItems = 3

            // Check if item has a menu
            modeOptionsBtn.modeData = modeData

            // Does mode have an associated menu?
            var optionsMenuIsDefined = (typeof modeData.menuUrl !== "undefined") && (modeData.menuUrl !== null)
            currentModeHasOptions = optionsMenuIsDefined && (modeData.menuUrl.length > 0)
            modeOptionsBtn.visible = currentModeHasOptions
            if (currentModeHasOptions)
                nItems = 4
        }
    }

    // Open/close menu
    ToolBarButtonDelegate {
        id: openCloseMenuBton
        modeData: {"name": "openCloseMenu", "icon": "qrc:/qml/toolbar/icons/arrow_previous.svg"}
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
                modeData: {"name": "openCloseMenu", "icon": "qrc:/qml/toolbar/icons/arrow_next.svg"}
            }
        }
    }

    // Cosneta logo
    ToolBarButtonDelegate {
        id: cosnetaBtn
        modeData: {"name": "cosneta", "icon": "qrc:/qml/toolbar/icons/app_menu_closed.svg"}
        onToolBarItemClicked: {
            if (root.state === "") {
                nItems = 2
                root.state = "compact"
                closeLeftWing()
                hideSubMenu()
            }
            else {
                setCurrentMode(currentMode)
                root.state = ""
                openLeftWing()
                hideSubMenu()
            }
        }
    }

    // Mode options
    ToolBarButtonDelegate {
        id: modeOptionsBtn
        modeData: {"name": "modeOptions", "icon": "qrc:/qml/toolbar/icons/question.svg"}
        onToolBarItemClicked: root.toolBarItemClicked(modeData)
        visible: currentModeHasOptions
        borderVisible: true
    }

    // Camera button
    ToolBarButtonDelegate {
        id: cameraBtn
        modeData: {"name": "grab", "icon": "qrc:/qml/toolbar/icons/snapshot.svg"}
        onToolBarItemClicked: root.toolBarItemClicked(modeData)
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
            modeData: {"name": "cosneta", "icon": "qrc:/qml/toolbar/icons/app_menu_open.svg"}
        }
    }
}
