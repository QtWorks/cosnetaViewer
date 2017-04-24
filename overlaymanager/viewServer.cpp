#include "viewServer.h"

#include "overlay.h"

#ifdef Q_OS_LINUX
#include <signal.h>
#endif

#define INCLUDE_CHANGE_MESSAGES
#define CHECK_NUM_ADDED

// Debug
#define DEBUG_COUNT
// #define DEBUG_START_STOP

#ifdef DEBUG_COUNT
static int  num_zero_len_tiles = 0;
#endif
#ifdef DEBUG_START_STOP
static bool sent_data_last_time = false;
#endif

ViewServer::ViewServer(qintptr skt, screenGrabber *scnGrab, QObject *parent) : QThread(parent)
{
#ifdef Q_OS_LINUX
    signal(SIGPIPE, SIG_IGN);  // Don't abort on a remote socket disconnecting...
#endif

    waitIngForParentToLetMeDie = false;
    connectedToClient          = false;
    allowRemoteViews           = false;
    socketDesc                 = skt;
    screenGrabberTask          = scnGrab;
    lastAckIndex               = 0;
    sendCount                  = 0;
    clientWantsView            = true;
    clientDataPreloaded        = false;
    initComplete               = false;
    queuedCount                = 0;
    hasBeenStopped             = true;

    lastSentSmallUpdateIndex.clear();
    lastSentBigUpdateIndex.clear();

    numClientLocks = 0; // Debug
}

ViewServer::~ViewServer()
{
    qDebug() << "Destroy viewserver.";
}


QString ViewServer::connectedHostName(void)
{
    return viewClientIPAddress;
}


void ViewServer::queueData(QByteArray *data)
{
    if( ! waitIngForParentToLetMeDie )
        sendQueue.enqueue(data);

    queuedCount += data->size();
}


int ViewServer::currentQueueSize(void)
{
    return queuedCount - sendCount;
}

void ViewServer::clientDoesNotWantView(void)
{
    clientWantsView = false;
}

// If only we could get some...
void ViewServer::handleDataReturnedFromClient(quint32 in)
{
//    qDebug() << "Client message 0x" << QString("%1").arg(in,0,16);

    switch( (in>>24)&255 )
    {
    case ACTION_ACKNOWLEDGE:

        lastAckIndex = in  & 0x00FFFFFF;
//        qDebug() << "Ack:"  << lastAckIndex << "sendCount" << sendCount/256
//                 << "diff:" << (sendCount/256-lastAckIndex);
        {
        QTime now              = QTime::currentTime();
        int   instantSendDelay = lastRxTime.msecsTo(now);

        if( instantSendDelay < 10 )
        {
            dataRxAtLastTimePoint += 256;
        }
        else
        {
            float latestRemoteRxRate = 4000.0*dataRxAtLastTimePoint/instantSendDelay;
            if( meanRemoteRxRate<0 )
            {
                meanRemoteRxRate = latestRemoteRxRate;
            }
            else
            {
                meanRemoteRxRate = (meanRemoteRxRate*0.95 + latestRemoteRxRate*0.05);
            }
//            qDebug() << "Mean remote Rx rate" << meanRemoteRxRate;

            lastRxTime            = now;
            dataRxAtLastTimePoint = 256;
        }
        }
        break;

    case ACTION_MENU_OPT:

        qDebug() << "ACTION_MENU_OPT: TODO write code to handle this. Client:" << viewClientIPAddress;
        break;

    default:

        qDebug() << "Unexpected message type:" << ACTION_DEBUG((in>>24)&255)
                 << QString("Data: 0x%1").arg(in,0,16) << "from" << viewClientIPAddress;
    }
}

void  ViewServer::waitForInitComplete(void)
{
    while( ! initComplete ) QThread::msleep(20);
}

void  ViewServer::queueIsNowPreloaded(void)
{
    clientDataPreloaded = true;
}


void ViewServer::run(void)
{
    int         currentSmallTileIndex = -1;
    int         currentBigTileIndex   = -1;
    quint8      rx[sizeof(quint32)];

    QByteArray *msg;

    qDebug() << "viewServer for" << viewClientIPAddress << "started.";

    // Start and connect a receiver socket

    viewTcpServer  = new QTcpSocket();

    // Register disconnected to provide another exit for this thread
    connect(viewTcpServer, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));

    if( ! viewTcpServer->setSocketDescriptor(socketDesc,
                                             QAbstractSocket::ConnectedState, QIODevice::ReadWrite) )
    {
        qDebug() << "Failed to open socket in child ViewServer:" << viewTcpServer->error();
    }
    else
    {
        viewTcpServer->setSocketOption(QAbstractSocket::LowDelayOption,1);

        connectedToClient = true;
    }

    viewClientIPAddress = viewTcpServer->peerAddress().toString();

    // Release any process waiting on me getting to this point

    initComplete = true;

    // Wait until the initial state has been loaded into the queue

    while( ! clientDataPreloaded ) QThread::msleep(20);

    // Maintain a count of screenGrabber clinets (mainly for debug at present)

    screenGrabberTask->addAUser();

    if( viewTcpServer->state() != QAbstractSocket::ConnectedState )
    {
        qDebug() << "Internal error: started server before entering connected state!";
    }

    lastSendTime            = QTime::currentTime();
    dataSentAtLastTimePoint = 0;
    lastRxTime              = QTime::currentTime();
    dataRxAtLastTimePoint   = 0;
    meanSendRate            = -1.0;
    meanRemoteRxRate        = -1.0;
    lastTileWasBigTile      = false;

    // Main loop: forever, wait until we have new data then send it to the client.
    while( connectedToClient )
    {
        // Receive Acks etc
        if( viewTcpServer->bytesAvailable() >= (int)sizeof(quint32) )
        {
            if( sizeof(quint32) == viewTcpServer->read((char *)rx, sizeof(quint32)) )
            {
                handleDataReturnedFromClient( rx[0]<<24 | rx[1]<<16 | rx[2]<<8 | rx[3] );
            }
        }

        // Transmit
        if( sendQueue.isEmpty() )
        {
            if( clientWantsView )
            {
                // That is, append view stuff to the send queue
                constructViewMessage(currentSmallTileIndex, currentBigTileIndex);
            }
            else
            {
                // Longer wait here as we're sending data a LOT less frequently.
                QThread::msleep(50);

                qDebug() << "No data sent";
            }
        }
        else // if( ((sendCount-lastAckIndex) & 0x00FFFFFF) < 256*32 ) - even with this, it still blocks
        {
            // Block if too much of a delay in receiving an ack, which will mean
            // that the last sent index vaguely relates to the client state
            msg = sendQueue.dequeue();
#if 0
            if( (sendCount % 1000) == 0 )
                qDebug() << "Send: sendCount =" << sendCount
                         << "lastAckIndex ="    << lastAckIndex
                         << "rx avail:"         << viewTcpServer->bytesAvailable()
                         ; // << " Socket state:"    << viewTcpServer->state();
#endif
            viewTcpServer->write(*msg);
//            viewTcpServer->flush();

            if( ! viewTcpServer->waitForBytesWritten() )
            {
                if( viewTcpServer->state() != QAbstractSocket::ConnectedState )
                {
                    connectedToClient = false;
                }
            }
            else
            {
                if( (sendCount & 255) == 0 )
                {
                    QTime now              = QTime::currentTime();
                    int   instantSendDelay = lastSendTime.msecsTo(now);

                    if( instantSendDelay < 10 )
                    {
                        dataSentAtLastTimePoint += 256;
                    }
                    else
                    {
                        float latestSendRate = 4000.0*dataSentAtLastTimePoint/instantSendDelay;
                        if( meanSendRate<0 )
                        {
                            meanSendRate = latestSendRate;
                        }
                        else
                        {
                            meanSendRate = (meanSendRate*0.95 + latestSendRate*0.05);
                        }
//                        qDebug() << "Mean send rate" << meanSendRate;
//                        qDebug() << "Mean send rate" << meanSendRate << "Mean remote Rx rate" << meanRemoteRxRate;

                        lastSendTime            = now;
                        dataSentAtLastTimePoint = 256;
                    }
                }

                sendCount = (sendCount + 1) & 0x00FFFFFF;
            }

            // We were passed a pointer and must delete the object here
            delete msg;
        }
    }

    qDebug() << "ViewServer for" << connectedHostName() << "is exiting...";

    disconnect(viewTcpServer, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));

    screenGrabberTask->releaseAUser();

    // Done

    qDebug() << "TODO: ensure that the viewServer object is deleted...";

    waitIngForParentToLetMeDie = true;

    emit IAmDead();

    if( viewTcpServer->state() == QAbstractSocket::ConnectedState )
    {
        viewTcpServer->waitForBytesWritten();
    }
    viewTcpServer->close();

    while( waitIngForParentToLetMeDie )
    {
        QThread::yieldCurrentThread();
    }

    qDebug() << "delete viewTCPServer";
    delete viewTcpServer;
    viewTcpServer->deleteLater();

    qDebug() << "viewServer thread exit. numClientLocks =" << numClientLocks;

    hasBeenStopped = true;
}

void ViewServer::youMayNowDie(void)
{
    qDebug() << "This is a horrible hack to try and trace the freeStyleQt crashes.";
//    waitIngForParentToLetMeDie = false;     // Don't detach as it's ours, and we may be restarted.
}

bool  ViewServer::waitingToDie(void)
{
    return waitIngForParentToLetMeDie;
}



int ViewServer::generateRepeatsMessage(QByteArray *message, int &currentSmallTileIndex)
{
    // Construct a repeat message
    int  numRepeatsInMessage = 0;
    message->resize(4); // Insert space at the start for the length

//        QString repeatsDbg = QString("Last %1 repeated at: ").arg(currentTileIndex);

    do
    {
        if( repeatList[0]<0 )
        {
            //qDebug() << "RepeatList entry" << repeatList[0] << "less than zero!";
            repeatList[0] = 0; // Hack
        }
        else if( repeatList[0]>=lastSentSmallUpdateIndex.size() )
        {
//            qDebug() << "RepeatList entry" << repeatList[0]
//                     << "larger than array:" << lastSentUpdateIndex.size();

            int oldSize = lastSentSmallUpdateIndex.size();
            lastSentSmallUpdateIndex.resize(repeatList[0]+1);
            for(int i=oldSize; i<lastSentSmallUpdateIndex.size(); i++) lastSentSmallUpdateIndex[i] = 0;
        }

        if( repeatList[0]                           != currentSmallTileIndex   &&
            lastSentSmallUpdateIndex[repeatList[0]] != repeatListUpdateNum[0]    )
        {
            numRepeatsInMessage ++;

            message->append( ( repeatList[0] >> 8 ) & 255 );
            message->append( ( repeatList[0]      ) & 255 );

//                repeatsDbg += QString("%1 ").arg(repeatList[0]);

            lastSentSmallUpdateIndex[repeatList[0]] = repeatListUpdateNum[0];
        }

        repeatListUpdateNum.remove(0);
        repeatList.remove(0);
    }
    while( repeatList.size()  >  0                          &&
           message->size()    <= (REPEAT_MESSAGE_MAX_SIZE - 2) );

//    repeatList.squeeze();
//    repeatListUpdateNum.squeeze();

//        qDebug() << repeatsDbg;

    message->data()[0] = ACTION_BACKGROUND_SAME_TILE;
    message->data()[1] = ( numRepeatsInMessage >> 16 ) & 255;
    message->data()[2] = ( numRepeatsInMessage >>  8 ) & 255;
    message->data()[3] = ( numRepeatsInMessage       ) & 255;

    return numRepeatsInMessage;
}


bool ViewServer::generateSmallTileMessages(QByteArray *message,
                                           int currentBigTileIndex, int &currentSmallTileIndex)
{
    // Recover next tile with list of repeats
    // Look for a tile that is out of date.
    bool    changeFound;
    int     startTileIndex = currentSmallTileIndex;
    int     updateNum;
    message->resize(8); // Insert space at the start for the length
#ifdef DEBUG_COUNT
    if( currentSmallTileIndex == 0 && num_zero_len_tiles > 0 )
    {
        qDebug() << "Last small tile scan resulted in" << num_zero_len_tiles << "zero length tiles (8160)";

        num_zero_len_tiles = 0;
    }
#endif
    do
    {
        updateNum = screenGrabberTask->nextSmallIndexesUpdateNum(currentSmallTileIndex);

        if( currentSmallTileIndex >= lastSentSmallUpdateIndex.size() )
        {
//            qDebug() << "Expand lastSentSmallUpdateIndex to" << (currentSmallTileIndex+1);

            lastSentSmallUpdateIndex.resize(currentSmallTileIndex+1);
            changeFound = true;
        }
        else if( updateNum != lastSentSmallUpdateIndex[currentSmallTileIndex] )
        {
            changeFound = true;
        }
    }
    while(startTileIndex != currentSmallTileIndex  && ! changeFound );

    if( ! changeFound )
    {
        // Nothing to send
#ifdef DEBUG_START_STOP
        if( sent_data_last_time ) qDebug() << "Stop sending data.";

        sent_data_last_time = false;
#endif
        return false;
    }
#ifdef DEBUG_START_STOP
    if( ! sent_data_last_time )
    {
        qDebug() << "Start sending data.";

        sent_data_last_time = true;
    }
#endif

    lastSentSmallUpdateIndex[currentSmallTileIndex] = updateNum;

    // We have a pending send tile
    QByteArray *blob = screenGrabberTask->grabSmallTileData(currentSmallTileIndex,updateNum);
    message->append( blob->constData(), blob->length() );

    if( message->size() < 9 )
    {
#ifdef DEBUG_COUNT
        if( screenGrabberTask->grabSmallTileData(currentSmallTileIndex,updateNum)->size() == 0 )
        {
            num_zero_len_tiles ++;
        }
        else
        {
#endif
            qDebug() << "Received a bad data size for small tile" << currentSmallTileIndex << "Sz ="
                     << screenGrabberTask->grabSmallTileData(currentSmallTileIndex,updateNum)->size();
#ifdef DEBUG_COUNT
        }
#endif
        // Nothing to send

        return false;
    }

    // TODO: tell screenGrabber that we're no longer using *blob (shouldn't really be an issue, but safer)

    screenGrabberTask->getRepeatsAndUpdateIndex(currentSmallTileIndex, repeatList, repeatListUpdateNum);

    int imageSize = message->size() - 8;
#ifdef PNG_COMPRESS_TILES
    message[0] = ACTION_BG_PNG_TILE;
    message[1] = ( imageSize >> 16 );
#else
    message->data()[0] = ACTION_BACKGROUND_BA_COMPRESSED;
    message->data()[1] = ( imageSize >> 16 ) & 0x0F | (BG_IMG_TILE_POW<<4);
#endif
    message->data()[2] = ( imageSize >>  8 ) & 0xFF;
    message->data()[3] = ( imageSize       ) & 0xFF;

    int x, y;
    screenGrabberTask->screenCoordsOfSmallTile(currentSmallTileIndex, x, y);

    message->data()[4] = (x >> 8) & 255;
    message->data()[5] =  x       & 255;
    message->data()[6] = (y >> 8) & 255;
    message->data()[7] =  y       & 255;

    return true;
}


bool ViewServer::generateBigTileMessages(QByteArray *message, int &currentBigTileIndex)
{
    int  xBig, yBig;
    int  updateNumber;

    screenGrabberTask->nextBigTileIndexAndCoords(currentBigTileIndex, updateNumber, xBig, yBig);

    // Allow for changing array sizes
    if( currentBigTileIndex >= lastSentBigUpdateIndex.size() )
    {
//        qDebug() << "Expand lastSentBigUpdateIndex to" << (currentBigTileIndex+1);

        int lastSize = lastSentBigUpdateIndex.size();

        lastSentBigUpdateIndex.resize(currentBigTileIndex+1);

        for(int i=lastSize; i<=currentBigTileIndex; i++) lastSentBigUpdateIndex[i] = -1;
    }

    // Sent this one already?
    if( updateNumber == lastSentBigUpdateIndex[currentBigTileIndex] )
    {
        return false;
    }

    // Count the sub tiles: only send if > 50% are out of date
    int           outOfDateCount;
    QVector<bool> outOfDate;

    outOfDateCount = screenGrabberTask->countOutOfDateSmallTiles(currentBigTileIndex,
                                                                 lastSentSmallUpdateIndex,
                                                                 outOfDate);

//    if( outOfDateCount < (screenGrabberTask->numSmallTilesPerBigXorY/2) )
    if( outOfDateCount < 3 )
    {
        // Less than "annoying" out of date, so no big tile please.
        return false;
    }

    // //////////////////////////////////////////////////////
    // Generate a big tile message (NB this does not repeat.)

    message->resize(8); // Insert space at the start for the length

    // Start with the mask (ie a bitfield indicating which subtiles should be untouched)
    quint8 byte = 0;
    for( int bit=0; bit<outOfDate.size(); bit ++ )
    {
        if( outOfDate[bit] ) byte |= 1<<(bit & 7);

        // NOTE: we assume that any mask is a multiple of 8 (otherwise we'll loose the end).
        if( (bit & 7) == 7 )
        {
            message->append(byte);
            byte = 0;
        }
    }

    // Should never be required, but what's life without some paranoia ;)
    if( (8 + screenGrabberTask->numSmallTilesPerBigXorY*screenGrabberTask->numSmallTilesPerBigXorY/8)
            !=  message->size()  )
    {
        qDebug() << "Big tile: Bad big message length:" << message->size() << "expected:"
                 << (8 + screenGrabberTask->numSmallTilesPerBigXorY*screenGrabberTask->numSmallTilesPerBigXorY/8);
    }
    if( message->size() & 3 )
    {
        qDebug() << "Big tile: Warning, bad mask size:" << message->size();
        message->resize( (message->size() + 3) & (~3) );
    }

    QByteArray jpgImage = *( screenGrabberTask->grabBigTileData(currentBigTileIndex,updateNumber) );
    int        imageSize = jpgImage.size();

    if( imageSize<119 )
    {
        qDebug() << "Big tile" << currentBigTileIndex << ": Smaller than the smallest valid jpeg. Sz =" << imageSize;
        return false;
    }

    message->data()[0] = ACTION_BG_JPEG_TILE;
    message->data()[1] = ( imageSize >> 16 ) & 0x0F | ((screenGrabberTask->numBigPerSmallPower)<<4);

    message->data()[2] = ( imageSize >>  8 ) & 0xFF;
    message->data()[3] = ( imageSize       ) & 0xFF;

    message->data()[4] = (xBig >> 8) & 255;
    message->data()[5] =  xBig       & 255;
    message->data()[6] = (yBig >> 8) & 255;
    message->data()[7] =  yBig       & 255;

    // Append the actual image data
    message->append( jpgImage );

//    qDebug() << "BIG jpeg: image size"     << imageSize
//             << "Message size:"            << message.size()
//             << "Coords:"                  << xBig << yBig
//             << "currentBigTileIndex"      << currentBigTileIndex
//             << "lastSentBigUpdateIndex[]" << lastSentBigUpdateIndex[currentBigTileIndex]
//             << "updateNumber"             << updateNumber;

    // Let's not send this again (unless it changes)
    lastSentBigUpdateIndex[currentBigTileIndex] = updateNumber;

    // Paranoia
    repeatList.clear();

    return true;
}



//
void ViewServer::constructViewMessage(int &currentSmallTileIndex, int &currentBigTileIndex)
{
    QByteArray *message = new QByteArray;

    if( repeatList.size() > 0 )
    {
        if( generateRepeatsMessage(message, currentSmallTileIndex) < 1 )
        {
            // No repeat entries
            QThread::yieldCurrentThread();
            return;
        }
    }
    else // Not a repeat message
    {
        lastTileWasBigTile = false;

        if( lastTileWasBigTile )
        {
            lastTileWasBigTile = false;

            if( ! generateSmallTileMessages(message, currentBigTileIndex, currentSmallTileIndex) )
            {
                QThread::yieldCurrentThread();
                return;
            }

            qDebug() << "TCP: Small Tile:" << currentSmallTileIndex << "Message size:"
                     << message->size()    << "# repeats:"          << repeatList.size();
        }
        else
        {
            lastTileWasBigTile = true;

            if( ! generateBigTileMessages(message, currentBigTileIndex) )
            {
                if( ! generateSmallTileMessages(message, currentBigTileIndex, currentSmallTileIndex) )
                {
                    QThread::yieldCurrentThread();
                    return;
                }
            }
            else
            {
//                qDebug() << "Big Tile:" << currentBigTileIndex << "Message size:" << message.size();
            }
        }
    }

    // Pad out to a 32 bit word
    message->resize( (message->size() + 3) & (~3) );
    if( (message->size() & 3) != 0 ) qDebug() << "Bad padding! Size =" << message->size();

    sendQueue.enqueue(message);
}


void ViewServer::clientDisconnected()
{
    connectedToClient = false;

    qDebug() << "ViewServer: Client disconnected.";
}
