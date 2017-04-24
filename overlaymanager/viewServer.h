#ifndef VIEWSERVER_H
#define VIEWSERVER_H

#include <QObject>
#include <QThread>
#include <QQueue>
#include <QTcpSocket>
#include <QByteArray>
#include <QTime>

#include "screenGrabber.h"

#define MAX_TRACKED_LOST_PACKETS 5

#define INVALID_MESSAGE_INDEX 0xFFFFFFFF

typedef struct
{
    quint32       update_num;
    union
    {
        quint32       msg_type;
        struct view_ack_s
        {
            quint32   msg_type;
            quint32   num_lost;
            quint32   lost_index[MAX_TRACKED_LOST_PACKETS];

        }             view_ack;
        unsigned char data[MAX_TRACKED_LOST_PACKETS*sizeof(quint32)];

    } body;

} udp_view_packet_t;

#define BROADCAST_VIEW_PORT              14756
#define BROADCAST_VIEW_PROBE_PORT        14757
#define MAX_ACCEPTED_DATAGRAM_SZ         32767
#define BROADCAST_VIEW_PROBE_MESSAGE_SZ   1024

#define REPEAT_MESSAGE_MAX_SIZE (sizeof(quint32)*256)

class ViewServer : public QThread
{
    Q_OBJECT
public:
    explicit         ViewServer(qintptr skt, screenGrabber *scnGrab, QObject *parent = 0);
                    ~ViewServer();

    void             run();
    void             constructViewMessage(int &currentSmallTileIndex, int &currentBigTileIndex);
//    void             constructViewMessage(int &currentTileIndex, bool &sendRepeatMessage, int &updateNum);
//    void             stopCommand(void);
    void             youMayNowDie(void);
    bool             waitingToDie(void);

    void             queueData(QByteArray *data);
    int              currentQueueSize(void);    // Allows sender to hold off if data is backing up.

    void             waitForInitComplete(void);
    void             queueIsNowPreloaded(void);

    QString          connectedHostName(void);

    void             clientDoesNotWantView(void);

    bool             hasBeenStopped;

signals:
    void             IAmDead(void);
    
public slots:

    void             clientDisconnected();

private:

    int              generateRepeatsMessage(QByteArray *message, int &currentSmallTileIndex);
    bool             generateSmallTileMessages(QByteArray *message, int currentBigTileIndex, int &currentSmallTileIndex);
    bool             generateBigTileMessages(QByteArray *message, int &currentBigTileIndex);
    void             handleDataReturnedFromClient(quint32 in);

    QTcpSocket      *viewTcpServer;
    QString          viewClientIPAddress;
    bool             clientDataPreloaded;
    bool             initComplete;

    QQueue<QByteArray *> sendQueue;
    bool             allowRemoteViews;
    qintptr          socketDesc;
    int              lastAckIndex;
    int              sendCount;
    int              queuedCount;
    bool             connectedToClient;
    screenGrabber   *screenGrabberTask;
    QVector<int>     lastSentSmallUpdateIndex;
    QVector<int>     lastSentBigUpdateIndex;
    bool             lastTileWasBigTile;
    bool             waitIngForParentToLetMeDie;

    QVector<int>     repeatList;
    QVector<int>     repeatListUpdateNum;

    QTime            lastSendTime;
    int              dataSentAtLastTimePoint;
    float            meanSendRate;
    QTime            lastRxTime;
    int              dataRxAtLastTimePoint;
    float            meanRemoteRxRate;

    bool             clientWantsView;

    // Debug
    int  numClientLocks;
};

#endif // VIEWSERVER_H
