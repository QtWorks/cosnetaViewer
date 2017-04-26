// Application
#include "roommanager.h"
#include "room.h"

// Constructor
RoomManager::RoomManager(QObject *parent) : QAbstractListModel(parent),
    m_iCurrentRoomIndex(-1)
{
    // Add first room
    addRoom();
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

// Return role names
QHash<int, QByteArray> RoomManager::roleNames() const
{
    QHash<int, QByteArray> hRoleNames;
    hRoleNames[RoomName] = "roomName";
    return hRoleNames;
}

// Return count
int RoomManager::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return count();
}

// Return data
QVariant RoomManager::data(const QModelIndex &index, int role) const
{
    // Check index
    if (!index.isValid())
        return QVariant();

    // Check bounds
    if ((index.row() < 0) || (index.row() > (count()-1)))
        return QVariant();

    // Get room
    Room *pRoom = m_vRooms[index.row()];

    // Return value for role
    if (role == RoomName)
        return pRoom->name();

    return QVariant();
}

// Return current room index
int RoomManager::currentRoomIndex() const
{
    return m_iCurrentRoomIndex;
}

// Set current room index
void RoomManager::setCurrentRoomIndex(int iRoomIndex)
{
    if ((iRoomIndex >= 0) && (iRoomIndex < count()))
    {
        m_iCurrentRoomIndex = iRoomIndex;
        emit currentRoomIndexChanged();
    }
}

// Return current room
QObject *RoomManager::currentRoom() const
{
    if ((m_iCurrentRoomIndex >= 0) && (m_iCurrentRoomIndex < count()))
    {
        return m_vRooms[m_iCurrentRoomIndex];
    }
    return NULL;
}

// Add room
void RoomManager::addRoom()
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    Room *pRoom = new Room();
    QString sRoomName = QString("Room%1").arg(m_vRooms.size()+1);
    pRoom->setName(sRoomName);
    m_vRooms << pRoom;
    endInsertRows();
    m_iCurrentRoomIndex = rowCount()-1;
}

// Remove current room
void RoomManager::removeCurrentRoom()
{
    // Keep at least one desk
    if (count() > 1)
        removeRoom(m_iCurrentRoomIndex);
}

// Remove room
void RoomManager::removeRoom(int iRoomIndex)
{
    if ((iRoomIndex >= 0) && (iRoomIndex < count()))
    {
        bool bIsLastRoom = (iRoomIndex == count()-1);
        beginRemoveRows(QModelIndex(), iRoomIndex, iRoomIndex);
        Room *pRoom = m_vRooms.takeAt(iRoomIndex);
        delete pRoom;
        endRemoveRows();
        if (m_vRooms.size() == 1)
            setCurrentRoomIndex(0);
        else
        if (bIsLastRoom)
            setCurrentRoomIndex(m_vRooms.size()-1);
    }
}

// Return count
int RoomManager::count() const
{
    return m_vRooms.size();
}
