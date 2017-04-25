// Application
#include "roommanager.h"
#include "room.h"

// Constructor
RoomManager::RoomManager(QObject *parent) : QObject(parent)
{

}

// Startup
bool RoomManager::startup()
{
    return true;
}

// Shutdown
void RoomManager::shutdown()
{

}

// Return count
int RoomManager::count() const
{
    return m_vRooms.size();
}

// Add room
void RoomManager::addRoom()
{
    Room *pRoom = new Room();
    QString sRoomName = QString("Room").arg(m_vRooms.size()+1);
    pRoom->setName(sRoomName);
    emit countChanged();
}

// Remove room
void RoomManager::removeRoom(int iRoomIndex)
{
    if ((iRoomIndex >= 0) && (iRoomIndex < m_vRooms.size()))
    {
        Room *pRoom = m_vRooms.takeAt(iRoomIndex);
        delete pRoom;
    }
}
