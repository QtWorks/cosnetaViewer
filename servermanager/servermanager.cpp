// Qt
#include <QTimer>

// Application
#include "servermanager.h"
#define TIME_OUT 20
#define BROADCAST_PORT 14755
#define RADAR_SOCKET_PORT 14759
#define REMOTE_CONNECT_PORT 14753
#define MANUAL_REMOTE_DATA "Manual Remote Data"

// Constructor
ServerManager::ServerManager(QObject *parent) : QObject(parent),
    m_bManualConfig(false), m_bLookingForHost(true), m_iHostPortNumber(0),
    m_iHostViewPortNumber(0)
{
    // Initialize data packets
    initializeDataPackets();
}

// Startup
bool ServerManager::startup()
{
    // Start remote connection
    startRemoteConnection();
    return true;
}

// Shutdown
void ServerManager::shutdown()
{

}

// Initialize data packets
void ServerManager::initializeDataPackets()
{
    // Set up the ping data packet
    memset(&m_sDataPacket, 0, sizeof(m_sDataPacket));
    m_sDataPacket.messageType = NET_RADAR_PULSE;
    m_sDataPacket.messageTypeCheck = NET_RADAR_PULSE;
}

// Start remote connection
void ServerManager::startRemoteConnection()
{
    // Start the RADAR
    m_uRadarSocket.bind(RADAR_SOCKET_PORT, QUdpSocket::ShareAddress);
    qDebug() << "remoteConnectDialogue: bind() radarSocket" << m_uRadarSocket.socketDescriptor() << "to" << 14755;
    connect(&m_uRadarSocket, SIGNAL(readyRead()), this, SLOT(onServerResponded()));
    QTimer::singleShot(100, this, SLOT(onPingForServers()));
}

// Return server count
int ServerManager::serverCount() const
{
    return m_vFoundServerList.size();
}

// Return server names
QStringList ServerManager::serverNames() const
{
    QStringList lServerNames;
    if (!m_vFoundServerList.isEmpty()) {
        foreach (serverEntry_t sEntry, m_vFoundServerList)
            lServerNames << sEntry.serverName;
    }
    lServerNames << MANUAL_REMOTE_DATA;
    return lServerNames;
}

// Add server
void ServerManager::addServer(const serverEntry_t &sServer)
{
    m_vFoundServerList << sServer;
    emit serverCountChanged();
}

// Remove server
void ServerManager::removeServer(int iServerIndex)
{
    if ((iServerIndex >= 0) && (iServerIndex < m_vFoundServerList.size()))
    {
        m_vFoundServerList.removeAt(iServerIndex);
        emit serverCountChanged();
    }
}

// Server responded
void ServerManager::onServerResponded()
{
    // Extract the packet
    int iIncommingSz = m_uRadarSocket.pendingDatagramSize();

    quint16 uRxPort;
    char *pRxBuffer  = (char *)calloc(iIncommingSz,sizeof(char));
    net_message_t *pRxMessage = (net_message_t *)pRxBuffer;
    QHostAddress hRxAddress;

    if (m_uRadarSocket.readDatagram(pRxBuffer, iIncommingSz, &hRxAddress, &uRxPort) == sizeof(net_message_t))
    {
        if(pRxMessage->messageType == NET_RADAR_ECHO)
        {
            // Is there a list entry for this already? In range => found (will be off end if not)
            int iIndex;
            for (iIndex=0; iIndex<m_vFoundServerList.length(); iIndex++)
                if (m_vFoundServerList[iIndex].IPAddress == hRxAddress)
                    break;

            // Just in case...
            pRxMessage->msg.radar.session_name[SESSION_NAME_SZ-1] = (char)0;

            // Retrieve server name
            QString sServerName = QString(pRxMessage->msg.radar.session_name);

            if (iIndex >= m_vFoundServerList.length())
            {
                // New entry
                serverEntry_t newServer;

                newServer.IPAddress = hRxAddress;
                newServer.serverName = sServerName;
                newServer.lastResponseTime = QTime::currentTime();

                // We want both lists to match in terms of index
                addServer(newServer);
            }
            else
            {
                // Entry already exists
                m_vFoundServerList[iIndex].serverName = sServerName;
                m_vFoundServerList[iIndex].lastResponseTime = QTime::currentTime();
            }
        }
    }

    // Stop receiving if we're not looking for a host
    if (!m_bLookingForHost)
        // Stop the RADAR (0 => all slots)
        disconnect(&m_uRadarSocket, SIGNAL(readyRead()), 0, 0);

    // Free
    free(pRxBuffer);
}

// Ping for servers
void ServerManager::onPingForServers()
{
    // Send a radar ping... and listen for the echos
    m_uRadarSocket.writeDatagram((char *)&m_sDataPacket, sizeof(m_sDataPacket),
                                 QHostAddress::Broadcast, BROADCAST_PORT);

    // Timeout any servers that we haven't heard from for a while
    // Give 'em 20 seconds as we don't ping very often and packets will be lost.
    for (int iIndex=(m_vFoundServerList.length()-1); iIndex>=0; iIndex --)
    {
        // Timeout after 20 seconds
        if (m_vFoundServerList[iIndex].lastResponseTime.secsTo(QTime::currentTime()) > TIME_OUT)
            // Timeout: delete this sever entry
            removeServer(iIndex);
    }

    // Keep polling while this dialogue is visible
    if (m_bLookingForHost)
    {
        int iDelay = 2000 + (qrand() % 1000);
        QTimer::singleShot(iDelay, this, SLOT(onPingForServers()));
    }
}

// Remote connect
void ServerManager::remoteConnect(int iServerIndex)
{
    if (iServerIndex <= m_vFoundServerList.size())
    {
        // Use discovered session
        m_hHostAddress = m_vFoundServerList[iServerIndex].IPAddress;
        m_iHostPortNumber = REMOTE_CONNECT_PORT;
        m_hHostViewAddress = m_hHostAddress;
        m_iHostViewPortNumber = m_iHostPortNumber + 1;
    }
    // Manual remote data
    else
    {

    }
}
