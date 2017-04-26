#ifndef ROOMMANAGER_H
#define ROOMMANAGER_H

// Qt
#include <QObject>
#include <QVector>
#include <QAbstractListModel>

// Application
#include "roommanager_global.h"
#include <iservice.h>
class Room;

class ROOMMANAGERSHARED_EXPORT RoomManager : public QAbstractListModel, public IService
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int currentRoomIndex READ currentRoomIndex WRITE setCurrentRoomIndex NOTIFY currentRoomIndexChanged)
    Q_PROPERTY(QObject *currentRoom READ currentRoom NOTIFY currentRoomIndexChanged)

public:
    enum Roles {RoomName=Qt::UserRole+1};

    // Constructor
    RoomManager(QObject *parent=NULL);

    // Startup
    virtual bool startup();

    // Shutdown
    virtual void shutdown();

    // Return role names
    virtual QHash<int, QByteArray> roleNames() const;

    // Return number of rooms
    virtual int rowCount(const QModelIndex &parent=QModelIndex()) const;

    // Return data
    virtual QVariant data(const QModelIndex &index, int role) const;
    
    // Return current room index
    int currentRoomIndex() const;

    // Set current room index
    void setCurrentRoomIndex(int iRoomIndex);

    // Return current room
    QObject *currentRoom() const;

    // Add room
    Q_INVOKABLE void addRoom();

    // Remove current room
    Q_INVOKABLE void removeCurrentRoom();

private:
    // Remove room
    void removeRoom(int iRoomIndex);

    // Return count
    int count() const;

private:
    // Rooms
    QVector<Room *> m_vRooms;

    // Current room index
    int m_iCurrentRoomIndex;

signals:
    // Count changed
    void countChanged();

    // Current room index changed
    void currentRoomIndexChanged();
};

#endif // ROOMMANAGER_H
