// Application
#include "room.h"

// Constructor
Room::Room(QObject *parent) : QObject(parent),
    m_sName("")
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
