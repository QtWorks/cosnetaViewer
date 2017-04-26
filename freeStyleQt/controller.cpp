// Qt
#include <QTimer>
#include <qqml.h>

// Application
#include "controller.h"
#include <servermanager.h>
#include <roommanager.h>

// Constructor
Controller::Controller(QObject *parent) : QObject(parent),
    m_pServerManager(NULL), m_pRoomManager(NULL)
{
    // Register type
    qmlRegisterType<Controller>("components", 1, 0, "Controller");

    // Build server manager
    m_pServerManager = new ServerManager(this);

    // Build room manager
    m_pRoomManager = new RoomManager(this);
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

// Return room manager
QObject *Controller::roomManager() const
{
    return m_pServerManager;
}
