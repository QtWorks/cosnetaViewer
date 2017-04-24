#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H

#include <QWidget>
#include <QThread>
#include <QMutex>

#include "viewServer.h"
#include "overlay.h"
#include "penCursor.h"
#include "stickyNote.h"
#include "remoteConnectDialogue.h"

// #define RECREATE_STICKY_NOTES_LOCALLY

class networkClient : public QObject
{
    Q_OBJECT
public:
    explicit networkClient(QImage     *drawImage,     QImage  *highlightImage, QImage  *txtImage,
                           penCursor  *penCursorIn[], QWidget *mainWindow,     QObject *parent = 0);
    ~networkClient();

    void   startReceiveView(int localPen, int remotePenNum, QImage *replayBackgroundImage, QHostAddress hostAddr, int hostPort);
    void   updateRemotePenNum(int newRemoteNum); // This allows reconnection with a new pen number
    void   stopReceiveView(void);
    void   haltNetworkClient(void);
    void   dontCheckForPenPresent(void);
    void   doCheckForPenPresent(void);
    void   enablePenReconnect(bool enable);
    void   setDecryptor(remoteConnectDialogue *newDecryptor);

signals:
    void   resolutionChanged(int w, int h);
    void   needsRepaint();
    void   moveReceivedPen(int penNum, int x, int y, bool show);
    void   remoteViewDisconnected();
    void   remotePenDisconnected();
    void   remoteViewAvailable();
    
public slots:
    void   doConnect(QThread &cThread);
    void   readThread();

    void   viewConnected();
    void   viewDisconnected();

private:

    void   displayTCPRemoteView(void);
    void   displayUDPBroadcastView(void);
    bool   negotiateBroadcastView(void);
    void   applyReceivedTCPWord(quint8 byte1, quint8 byte2, quint8 byte3, quint8 byte4);
    void   applyReceivedAction(int tileUpdateNum, unsigned char *packet, int len);
    void   applyReceivedBackgroundPNGImage(int updateNum, unsigned char *data, int length);
    void   applyReceivedBackgroundJPGImage(int updateNum, unsigned char *data, int length);
    void   drawLocalCopyOfRemoteCursors(bool *present, int *xLoc, int *yLoc);
    void   updateBackgroundImageTile(int updateNum, int x, int y, int length, unsigned char *data);
    void   updateBACompressedBackgroundImageTile(int updateNum, int x, int y, int sz, int length, quint8 *data);
//    void   updateBACompressedBackgroundImageTileWithRepeats(int updateNum, int x, int y, int sz, int length, int numRepeats, quint8 *repeatData, quint8 *imageData);
    void   updateJPEGCompressedBackgroundImageTile(int updateNum, int drawX, int drawY, int length, int newNumBigTilesPerSmall, quint8 *data);
    void   repeatBackgroundImageTile(int numRepeats, int repeatTileIndex, unsigned char *data);
    void   checkPenPresent(unsigned char *data);

    void   adjustUpdateTileArraySizes(void);
    void   storeSmallTileUpdateNum(int xInSmallTiles,int yInSmallTiles, int updateNum);
    void   storeBigTileUpdateNum(int xInBigTiles,int yInBigTiles, int updateNum);
    int    latestSmallTileUpdateNum(int xInSmallTiles,int yInSmallTiles);
    int    latestBigTileUpdateNum(int xInBigTiles,int yInBigTiles);

#ifdef RECREATE_STICKY_NOTES_LOCALLY
    QList<stickyNote *>    notes;
#endif
    int                    localPenNum;
    int                    penOnRemoteSystem;
    bool                   viewOnlyMode;
    int                    countDownBeforeStartChecking;

    int                    numBytesReceived;

    QHostAddress           hostViewAddr;
    int                    hostViewPort;
    QTcpSocket            *receiveSocket;
    bool                   connectedToHost;
    bool                   reconnectRemotePenOnDisconnect;
    bool                   doNotExit;
    QByteArray             rxStream;
    bool                   viewingReceivedStream;

    QUdpSocket            *broadcastSocket;
    bool                   broadcastView;

    overlay               *receivedAnnotations;
    penCursor             *receivedPenCursor[MAX_SCREEN_PENS];  // Array of MAX_SCREEN_PENS; reference from parent.

    QWidget               *parentWindow;
    QImage                *behindTheAnnotations;
    int                    w;
    int                    h;
    QImage                *updateTile;
    int                    updateTileWidth, updateTileHeight;
    int                    updateTileUpdateNum;
    int                    numBigTilesPerSmall;
    QByteArray            *receivedImageData;
    int                    widthInSmallTiles;
    QVector<int>           smallTileUpdateNum;
    int                    lastSmallTileIndex;
    int                    widthInBigTiles;
    QVector<int>           bigTileUpdateNum;

    bool                   haveNotEmmitedConnected;
    int                    numberOfBigTilesReceived;
    int                    numberOfSmallTilesReceived;

    bool                   penPresent[MAX_SCREEN_PENS];
    int                    penPosPrevX[MAX_SCREEN_PENS];
    int                    penPosPrevY[MAX_SCREEN_PENS];

    QColor                 penColour[MAX_SCREEN_PENS];       // Allows save state to be saved
    int                    penThickness[MAX_SCREEN_PENS];
    int                    penDrawstyle[MAX_SCREEN_PENS];

    QMutex                *blockRunStart;
    bool                   keepConnectionRunning;

    quint32                seq_num;
    quint32                last_rx_seq_num;

    remoteConnectDialogue *decryptor;

};

#endif // NETWORKCLIENT_H
