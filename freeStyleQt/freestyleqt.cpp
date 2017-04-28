// Application
#include "freestyleqt.h"
#include "controller.h"
#include "baseview.h"

// Constructor
FreeStyleQt::FreeStyleQt()
{
    // Build controller
    m_pController = new Controller();

    // Build view
    m_pView = new BaseView();
    m_pView->setContextProperty("controller", m_pController);
    m_pView->setSource(QUrl("qrc:/qml/main.qml"));
}

// Startup
bool FreeStyleQt::startup()
{
    // Show view
    if (m_pController->startup()) {
        m_pView->showMaximized();
        return true;
    }
    return false;
}

// Shutdown
void FreeStyleQt::shutdown()
{

}
