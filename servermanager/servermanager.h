#ifndef SERVERMANAGER_H
#define SERVERMANAGER_H

// Qt
#include <QObject>
#include <QUdpSocket>
#include <QTime>

// Application
#include "../systrayqt/net_connections.h"
#include "iservice.h"
#include "servermanager_global.h"

// Describe a single server entry
typedef struct server_s
{
    QString serverName;
    QHostAddress IPAddress;
    QTime lastResponseTime;
}
serverEntry_t;

class SERVERMANAGERSHARED_EXPORT ServerManager : public QObject, public IService
{
    Q_OBJECT
    Q_PROPERTY(int serverCount READ serverCount NOTIFY serverCountChanged)
    Q_PROPERTY(QStringList serverNames READ serverNames NOTIFY serverCountChanged)

public:
    // Constructor
    explicit ServerManager(QObject *parent = 0);

    // Startup
    virtual bool startup();

    // Shutdown
    virtual void shutdown();

    // Remote connect
    Q_INVOKABLE void remoteConnect(int iServerIndex);

private:
    // Initialize data packets
    void initializeDataPackets();

    // Start remote connection
    void startRemoteConnection();

    // Return server count
    int serverCount() const;

    // Return server names
    QStringList serverNames() const;

    // Add server
    void addServer(const serverEntry_t &sServer);

    // Remove server
    void removeServer(int iServerIndex);

private:
    // Data packet
    net_message_t m_sDataPacket;

    // Radar socket
    QUdpSocket m_uRadarSocket;

    // Found server list
    QVector<serverEntry_t> m_vFoundServerList;

    // Host address
    QHostAddress m_hHostAddress;

    // Host port number
    int m_iHostPortNumber;

    // Host view address
    QHostAddress m_hHostViewAddress;

    // Host view port number
    int m_iHostViewPortNumber;

    // Manual config?
    bool m_bManualConfig;

    // Looking for host?
    bool m_bLookingForHost;

public slots:
    // Server responded
    void onServerResponded();

    // Ping for servers
    void onPingForServers();

signals:
    // Server count changed
    void serverCountChanged();
};

#endif // SERVERMANAGER_H
