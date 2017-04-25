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
    Q_PROPERTY(int currentMode READ currentMode WRITE setCurrentMode NOTIFY currentModeChanged)
    Q_ENUMS(AppMode)

public:
    enum AppMode {ViewMode=0, PrivateNotesMode, PublicAnnotationMode, PresentationMode, MouseMode};

    // Constructor
    Overlaymanager(QObject *parent=NULL);

    // Startup
    virtual bool startup();

    // Shutdown
    virtual void shutdown();

    // Set current mode
    void setCurrentMode(int iCurrentMode);

    // Return current mode
    int currentMode() const;

private:
    // Current mode
    AppMode m_eCurrentMode;

signals:
    // Current mode changed
    void currentModeChanged();
};

#endif // OVERLAYMANAGER_H
