// Qt
#include <qqml.h>

// Application
#include "room.h"

// Constructor
Room::Room(QObject *parent) : QObject(parent),
    m_sName(""), m_eCurrentMode(ViewMode), m_eCurrentDrawingMode(FreeHand)
{
    // Register type
    qmlRegisterType<Room>("Components", 1, 0, "Room");
}

// Destructor
Room::~Room()
{

}

// Return name
const QString &Room::name() const
{
    return m_sName;
}

// Set name
void Room::setName(const QString &sName)
{
    m_sName = sName;
    emit nameChanged();
}

// Return current mode
int Room::currentMode() const
{
    return (int)m_eCurrentMode;
}

// Set current mode
void Room::setCurrentMode(int iCurrentMode)
{
    m_eCurrentMode = (AppMode)iCurrentMode;
    emit currentModeChanged();
}

// Return current drawing mode
int Room::currentDrawingMode() const
{
    return (int)m_eCurrentDrawingMode;
}

// Set current drawing mode
void Room::setCurrentDrawingMode(int iCurrentDrawingMode)
{
    m_eCurrentDrawingMode = (DrawingMode)iCurrentDrawingMode;
    emit currentDrawingModeChanged();
}

