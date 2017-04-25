// Qt
#include <QTimer>
#include <qqml.h>

// Application
#include "controller.h"
#include <servermanager.h>

// Constructor
Controller::Controller(QObject *parent) : QObject(parent),
    m_pServerManager(NULL)
{
    // Register type
    qmlRegisterType<Controller>("components", 1, 0, "Controller");

    // Build server manager
    m_pServerManager = new ServerManager(this);
}

// Startup
bool Controller::startup()
{
    return m_pServerManager->startup();
}

// Shutdown
void Controller::shutdown()
{
    m_pServerManager->shutdown();
}

// Return server manager
QObject *Controller::serverManager() const
{
    return m_pServerManager;
}
