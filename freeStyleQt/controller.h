#ifndef CONTROLLER_H
#define CONTROLLER_H

// Qt
#include <QObject>
#include <QUdpSocket>

// Application
#include "iservice.h"
class ServerManager;
class RoomManager;

class Controller : public QObject, public IService
{
    Q_OBJECT
    Q_PROPERTY(QObject *serverManager READ serverManager NOTIFY serverManagerChanged)
    Q_PROPERTY(QObject *roomManager READ roomManager NOTIFY roomManagerChanged)

public:
    // Constructor
    explicit Controller(QObject *parent = 0);

    // Startup
    virtual bool startup();

    // Shutdown
    virtual void shutdown();

    // Return server manager
    QObject *serverManager() const;

    // Return room manager
    QObject *roomManager() const;

private:
    // Server manager
    ServerManager *m_pServerManager;

    // Room manager
    RoomManager *m_pRoomManager;

signals:
    // Server manager changed
    void serverManagerChanged();

    // Room manager changed
    void roomManagerChanged();
};

#endif // CONTROLLER_H
