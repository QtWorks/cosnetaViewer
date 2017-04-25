#ifndef ROOMMANAGER_H
#define ROOMMANAGER_H

// Qt
#include <QObject>
#include <QVector>

// Application
#include "roommanager_global.h"
#include <iservice.h>
class Room;

class ROOMMANAGERSHARED_EXPORT RoomManager : public QObject, public IService
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    // Constructor
    RoomManager(QObject *parent=NULL);

    // Startup
    virtual bool startup();

    // Shutdown
    virtual void shutdown();

    // Return number of rooms
    int count() const;

    // Add room
    void addRoom();

    // Remove room
    void removeRoom(int iRoomIndex);

private:
    // Rooms
    QVector<Room *> m_vRooms;

signals:
    // Count changed
    void countChanged();
};

#endif // ROOMMANAGER_H
