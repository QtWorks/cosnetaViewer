#ifndef VIEWBROADCASTER_H
#define VIEWBROADCASTER_H

#include <QThread>
#include <QUdpSocket>
#include <QMutex>

#include "viewServer.h"
#include "screenGrabber.h"


class viewBroadcaster : public QThread
{
    Q_OBJECT
public:
    explicit viewBroadcaster(screenGrabber *scnGrab, bool *penActiveArray, QObject *parent = 0);
    ~viewBroadcaster();

    void             run();

    void             pauseSending(void);
    void             resumeSending(void);
    void             penPresentReference(bool *penActive);
    void             setAverageDataRate(int newRateBitPerSec);
    double           getCurrentSendRateMbs(void);

signals:

public slots:

private:
    void             buildAndSendViewMessage(int &updateNum);
    int              generateRepeatsMessage(QByteArray &message, int &updateNum);
    bool             generateSmallTileMessages(QByteArray &message, int &updateNum);
    bool             generateBigTileMessages(QByteArray &message, int &updateNumber);
    int              numOutOfDateBigTiles(void);
    int              findBigIndexOfLostMessage(int messageNum); // <0 => not found
    bool             sendBigTileMessageFromUpdate(QByteArray &message, int &updateNumber, int &tileIndex, int &xBig, int &yBig);

    quint32          sentBytes;
    quint32          lastUpdateNumber;
    screenGrabber   *screenGrabberTask;
    bool             keepBroadcastServerAvailable;
    QUdpSocket      *sendSocket;
    QVector<int>     repeatList;
    QVector<int>     lastSentSmallUpdateIndex;
    QVector<int>     numResendsSmallUpdateIndex;
    QVector<int>     repeatListUpdateNum;
    int              currentSmallTileIndex;
    int              lastTileIndex;
    bool             lowDefRequired;
    int              lowDefRequiredStartIndex;
    int              currentBigTileIndex;
    QVector<int>     lastSentBigUpdateIndex;
    bool             newClientJoined;
    QList<quint32>   lastSendBigMessageNumber;
    QList<quint32>   lastSendBigMessageScreenIndex;
    bool            *penIsActive;
    bool             sendingFullscreenUpdate;
    QTime           *lastFullScreenSendTimer;
    int              fullscreenUpdateTileNum;
    int              dataRateBitPerSec;
    double           currentTxRateKbs;

    QMutex          *pause;

};

#endif // VIEWBROADCASTER_H
