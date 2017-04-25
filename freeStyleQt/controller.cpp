// Qt
#include <QTimer>
#include <qqml.h>

// Application
#include "controller.h"
#include <servermanager.h>

// Constructor
Controller::Controller(QObject *parent) : QObject(parent),
    m_pServerManager(NULL), m_eCurrentMode(ViewMode)
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

// Set current mode
void Controller::setCurrentMode(int iCurrentMode)
{
    m_eCurrentMode = (AppMode)iCurrentMode;
}

// Return current mode
int Controller::currentMode() const
{
    return (int)m_eCurrentMode;
}

