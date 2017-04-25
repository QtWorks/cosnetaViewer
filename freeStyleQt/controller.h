#ifndef CONTROLLER_H
#define CONTROLLER_H

// Qt
#include <QObject>
#include <QUdpSocket>

// Application
#include "iservice.h"
class ServerManager;

class Controller : public QObject, public IService
{
    Q_OBJECT
    Q_PROPERTY(QObject *serverManager READ serverManager NOTIFY serverManagerChanged)

public:
    // Constructor
    explicit Controller(QObject *parent = 0);

    // Startup
    virtual bool startup();

    // Shutdown
    virtual void shutdown();

    // Return server manager
    QObject *serverManager() const;

private:
    // Server manager
    ServerManager *m_pServerManager;

signals:
    // Server manager changed
    void serverManagerChanged();
};

#endif // CONTROLLER_H
