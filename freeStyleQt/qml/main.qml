import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import "./pages/login"

Item {
    id: root

    // General settings
    property variant gGeneralSettings

    // Page settings
    property variant gPageSettings

    // Toolbar settings
    property variant gToolBarSettings

    // Parse general settings
    JSONParser {
        id: generalSettings
        source: "qrc:/settings/generalsettings.json"
        onDataReady: {
            // Log
            console.log("Loaded general settings")

            // Parse page data
            gGeneralSettings = JSON.parse(responseText)

            // Parse page info
            pageParser.source = "qrc:/settings/pages.json"
        }
    }

    // Parse pages
    JSONParser {
        id: pageParser
        onDataReady: {
            // Log
            console.log("Loaded page settings")

            // Parse page data
            gPageSettings = JSON.parse(responseText)

            // Load app
            toolBarParser.source = "qrc:/qml/toolbar/json/toolbars.json"
        }
    }

    // Toolbar parser
    JSONParser {
        id: toolBarParser
        onDataReady: {
            // Log
            console.log("Loaded toolbar settings")

            // Parse toolbar data
            gToolBarSettings = JSON.parse(responseText)

            // Load main application
            mainApplicationLoader.sourceComponent = mainApplicationComponent
        }
    }

    // Main application component:
    Component {
        id: mainApplicationComponent
        MainApplication {
            id: gMainApplication
            anchors.fill: parent
        }
    }

    // Main application loader:
    Loader {
        id: mainApplicationLoader
        anchors.fill: parent
    }
}
