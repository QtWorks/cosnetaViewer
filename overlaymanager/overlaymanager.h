#ifndef OVERLAYMANAGER_H
#define OVERLAYMANAGER_H

// Qt
#include <QObject>

// Application
#include "overlaymanager_global.h"
#include <iservice.h>

class OVERLAYMANAGERSHARED_EXPORT Overlaymanager : public QObject, public IService
{
    Q_OBJECT

public:
    // Constructor
    Overlaymanager(QObject *parent=NULL);

    // Startup
    virtual bool startup();

    // Shutdown
    virtual void shutdown();
};

#endif // OVERLAYMANAGER_H
