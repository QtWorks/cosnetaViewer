// Application
#include "freestyleqt.h"
#include "controller.h"
#include "baseview.h"

// Constructor
FreeStyleQt::FreeStyleQt()
{
    // Build controller
    m_pController = ControllerPtr(new Controller());

    // Build view
    m_pView = ViewPtr(new BaseView());
    m_pView->setContextProperty("controller", m_pController.data());
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
