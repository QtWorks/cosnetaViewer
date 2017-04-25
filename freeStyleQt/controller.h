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
    Q_ENUMS(ViewMode)

public:
    enum AppMode {ViewMode=0, PrivateNotesMode, PublicAnnotationMode, PresentationMode, MouseMode};

    // Constructor
    explicit Controller(QObject *parent = 0);

    // Startup
    virtual bool startup();

    // Shutdown
    virtual void shutdown();

    // Return server manager
    QObject *serverManager() const;

    // Set current mode
    void setCurrentMode(int iCurrentMode);

    // Return current mode
    int currentMode() const;

private:
    // Server manager
    ServerManager *m_pServerManager;

    // Current mode
    AppMode m_eCurrentMode;

signals:
    // Server manager changed
    void serverManagerChanged();
};

#endif // CONTROLLER_H
