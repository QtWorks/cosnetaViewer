#ifndef ROOM_H
#define ROOM_H

// Qt
#include <QObject>

// Application
#include "roommanager_global.h"

class ROOMMANAGERSHARED_EXPORT Room : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

public:
    friend class RoomManager;

    // Return name
    const QString &name() const;

    // Set name
    void setName(const QString &sName);

protected:
    // Constructor
    explicit Room(QObject *parent = 0);

private:
    // Room name
    QString m_sName;

signals:
    // Name changed
    void nameChanged();

public slots:
};

#endif // ROOM_H
