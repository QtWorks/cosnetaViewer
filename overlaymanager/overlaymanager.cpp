// Application
#include "overlaymanager.h"

// Constructor
Overlaymanager::Overlaymanager(QObject *parent) : QObject(parent),
    m_eCurrentMode(ViewMode)
{
}

// Startup
bool Overlaymanager::startup()
{
    return true;
}

// Shutdown
void Overlaymanager::shutdown()
{

}

// Set current mode
void Overlaymanager::setCurrentMode(int iCurrentMode)
{
    m_eCurrentMode = (AppMode)iCurrentMode;
    emit currentModeChanged();
}

// Return current mode
int Overlaymanager::currentMode() const
{
    return (int)m_eCurrentMode;
}

