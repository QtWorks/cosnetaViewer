#include "networkClient.h"
#include "overlaywindow.h"

#include <QColor>
#include <QMessageBox>
#include <QMutexLocker>

// #define SHOW_RX_COUNT

// #define SHOW_ALL_RECEIVED_MESSAGES

// #define SHOW_LOST_PACKETS

// #define SHOW_TILE_DEBUG

networkClient::networkClient(QImage *drawImage, QImage  *highlightImage, QImage *txtImage,
                             penCursor *penCursorIn[MAX_SCREEN_PENS], QWidget *mainWindow,
                             QObject *parent ) : QObject(parent)
{
    QColor  defaultPenColour[9];

    // TODO: move this to main.cpp as we're moving static data there
    defaultPenColour[0] = Qt::red;        defaultPenColour[1] = Qt::green;
    defaultPenColour[2] = Qt::yellow;     defaultPenColour[3] = Qt::blue;
    defaultPenColour[4] = Qt::magenta;    defaultPenColour[5] = Qt::cyan;
    defaultPenColour[6] = Qt::black;      defaultPenColour[7] = Qt::white;
    defaultPenColour[8] = Qt::lightGray;

    // NB We pass in our parent as we are not a graphical entity
    receivedAnnotations = new overlay(drawImage,highlightImage,txtImage,defaultPenColour,Qt::transparent,false,NULL,mainWindow);

    w = drawImage->width();
    h = drawImage->height();

    updateTileWidth       = BG_IMG_TILE_SIZE;
    updateTileHeight      = BG_IMG_TILE_SIZE;

    updateTileUpdateNum   = 0;
    numBigTilesPerSmall   = 8;

    numberOfBigTilesReceived   = 0;
    numberOfSmallTilesReceived = 0;
    haveNotEmmitedConnected    = true;

    adjustUpdateTileArraySizes();

    behindTheAnnotations = NULL;

    for(int pen=0; pen<MAX_SCREEN_PENS; pen++)
    {
        penPresent[pen] = false;

        penColour[pen]    = defaultPenColour[pen];
        penThickness[pen] = 2;
        penDrawstyle[pen] = 1;

        receivedPenCursor[pen] = penCursorIn[pen];
        receivedPenCursor[pen]->hide();
        receivedPenCursor[pen]->updatePenColour( defaultPenColour[pen] );
    }

    behindTheAnnotations  = NULL;
    updateTile            = new QImage; // Created here as deleted before every use.
    viewingReceivedStream = false;
    parentWindow          = mainWindow;
    blockRunStart         = new QMutex(QMutex::NonRecursive);
    keepConnectionRunning = false;
    reconnectRemotePenOnDisconnect = true;
#ifdef SHOW_RX_COUNT
    numBytesReceived      = 0;
#endif

    seq_num               = 0;
    last_rx_seq_num       = 0;
    lastSmallTileIndex    = 99999999;

    receivedImageData     = new QByteArray; // Created here as deleted before every use.

    receiveSocket         = NULL;
    broadcastSocket       = NULL;           // Created in the read thread

    decryptor             = NULL;

    blockRunStart->lock();
}

networkClient::~networkClient()
{
    delete receivedImageData;
    delete updateTile;
    delete receiveSocket;
}


// Signals

void networkClient::doConnect(QThread &cThread)
{
    // Start the thread
    connect(&cThread, SIGNAL(started()), this, SLOT(readThread()) );
}

void networkClient::viewConnected()
{
    qDebug() << "# Host connection started.";

    connectedToHost         = true;
    haveNotEmmitedConnected = true;
}

void networkClient::viewDisconnected()
{
    qDebug() << "# Host disconnected.";

    connectedToHost = false;
    emit remoteViewDisconnected();
}

void networkClient::setDecryptor(remoteConnectDialogue *newDecryptor)
{
    qDebug() << "TODO: decrypt view stream.";
}




void networkClient::dontCheckForPenPresent()
{
    viewOnlyMode = true;
}


void networkClient::doCheckForPenPresent()
{
    viewOnlyMode                 = false;
    countDownBeforeStartChecking = 2;
}

void networkClient::updateRemotePenNum(int newRemoteNum)
{
    penOnRemoteSystem = newRemoteNum;
}


void networkClient::startReceiveView(int localPen, int remotePenNum, QImage *replayBackgroundImage,
                                     QHostAddress hostAddr, int hostPort)
{
    qDebug() << "# startReceiveView: connect to" << hostAddr << "port" << hostPort;

//    if( ! broadcastSocket->bind(BROADCAST_VIEW_PORT) )
//        qDebug() << "Failed to bind broadcast listener socket.";

    // Record the data to be used during the session
    behindTheAnnotations  = replayBackgroundImage;
    localPenNum           = localPen;
    penOnRemoteSystem     = -1;
    viewOnlyMode          = true;
    viewingReceivedStream = true;

    // Record the required host
    hostViewAddr = hostAddr;
    hostViewPort = hostPort;

    // And draw a synchronising message to a black screen.
    behindTheAnnotations->fill(Qt::black);
    replayBackgroundImage->fill(Qt::transparent);

    QPainter paint(behindTheAnnotations);
    QFont    font = paint.font();

    font.setPointSize(20);
    paint.setFont(font);

    paint.setRenderHint(QPainter::Antialiasing);
    paint.setPen(Qt::black);
    paint.drawText(QRect(0, 0,
                   behindTheAnnotations->width(), behindTheAnnotations->height()),
                   Qt::AlignCenter, tr("Awaiting session image..."));
    paint.setPen(Qt::white);
    paint.drawText(QRect(2, 2,
                   behindTheAnnotations->width(), behindTheAnnotations->height()),
                   Qt::AlignCenter, tr("Awaiting session image..."));
    paint.end();

    emit needsRepaint();

    blockRunStart->unlock();
}



void    networkClient::stopReceiveView(void)
{
    for( int penNum=0; penNum<MAX_SCREEN_PENS; penNum++ )
    {
        receivedPenCursor[penNum]->hide();
    }

    viewingReceivedStream = false;
    behindTheAnnotations  = NULL;

    disconnect(receiveSocket,SIGNAL(connected()),   this,SLOT(viewConnected()));
    disconnect(receiveSocket,SIGNAL(disconnected()),this,SLOT(viewDisconnected()));

    // Wait until done, switching on the lock
    qDebug() << "Block readThread() for when it exits the loop...";

    blockRunStart->lock();

    keepConnectionRunning = false;
    doNotExit             = true;

    qDebug() << "emit needsRepaint";

    emit needsRepaint();
}



bool networkClient::negotiateBroadcastView(void)
{
    if( broadcastSocket == NULL )
    {
        broadcastSocket = new QUdpSocket; //  udpSkt
        if( ! broadcastSocket->bind(QHostAddress::Any, BROADCAST_VIEW_PORT) ) // ,QUdpSocket::ShareAddress) )
            qDebug() << "Failed to bind network client broadcast socket to receive on" << BROADCAST_VIEW_PORT;
        qDebug() << "bind() networkClient broadcastSocket" << broadcastSocket->socketDescriptor() << "to port" << BROADCAST_VIEW_PORT << "in thread.";
        // Increasing this on the Motorola device does not reduce packet loss
        qDebug() << "broadcastSocket RxBuffSz:" << broadcastSocket->socketOption(QAbstractSocket::ReceiveBufferSizeSocketOption);
//        broadcastSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,1000000);
//        qDebug() << "broadcastSocket RxBuffSz:" << broadcastSocket->socketOption(QAbstractSocket::ReceiveBufferSizeSocketOption);
//        broadcastSocket->setSocketOption(QAbstractSocket::TypeOfServiceOption,32 /* Priority */);
    }

    char               buf[BROADCAST_VIEW_PROBE_MESSAGE_SZ];
    udp_view_packet_t *probeMessage = (udp_view_packet_t *)buf;
    udp_view_packet_t  probeResponse;

    probeMessage->update_num    = 0;
    probeMessage->body.msg_type = ACTION_PROBE_VIEW_BROADCAST;

    bool         rxdResp = false;
    QTime        tm;
    int          waitStart = 0;
    QHostAddress senderAddress;
    quint16      senderPort;

    tm.start();

    // Try the UDP negotiation up to 10 times
//    for(int negotiateAttempt=0; negotiateAttempt<20 && ! rxdResp; negotiateAttempt++)

    // Temp: only use UDP till working as it confuses the debug.
    while( ! rxdResp && keepConnectionRunning )
    {
//        qDebug() << "negotiate broadcast view attempt" // << negotiateAttempt
//                 << "socket state:" << broadcastSocket->state();

        // Send a pair (so the host can check bandwidth)
        broadcastSocket->writeDatagram(buf, BROADCAST_VIEW_PROBE_MESSAGE_SZ, QHostAddress::Broadcast,BROADCAST_VIEW_PROBE_PORT);
        broadcastSocket->waitForBytesWritten();
        broadcastSocket->writeDatagram(buf, BROADCAST_VIEW_PROBE_MESSAGE_SZ, QHostAddress::Broadcast,BROADCAST_VIEW_PROBE_PORT);
        broadcastSocket->waitForBytesWritten();
        probeMessage->update_num ++;

        waitStart = tm.elapsed();
        rxdResp   = false;

        // While loop to allow for "other" sources of UDP packets.
        while( ! rxdResp && (tm.elapsed() - waitStart) < 100 )
        {
            // Wait for a response (1/10 sec max)
            if( broadcastSocket->waitForReadyRead(100) )
            {
                int rxSize = broadcastSocket->readDatagram((char *)&probeResponse,  sizeof(udp_view_packet_t),
                                                          &senderAddress, &senderPort );
                if( rxSize == sizeof(udp_view_packet_t) )
                {
                    last_rx_seq_num = probeResponse.update_num;

                    // Packet is right length, is it an accept message?
                    if( probeResponse.body.msg_type == ACTION_VIEW_BROADCAST_ACK )
                    {
                        qDebug() << "Accepted response was" << rxSize << "bytes long.";
                        rxdResp = true;
                    }
#if 1
                    else if( (probeResponse.body.msg_type & 255) != ACTION_BACKGROUND_BA_COMPRESSED )
                    {
                        // Unexpected packet (we expect lots of BA compressed tiles
                        qDebug() << "Rx-ed packet from" << senderAddress
                                 << "while waiting for ACK. Was" << rxSize << "bytes. Type"
                                 << QString("0x%1").arg(probeResponse.body.msg_type & 255,0,16)
                                 << "wanted" << QString("0x%1").arg(ACTION_VIEW_BROADCAST_ACK,0,16);
                    }
#endif
                }
#if 1
                else qDebug() << "Rx-ed" << rxSize
                              << "bytes. Wanted sz" << sizeof(probeResponse)
                              << ". Type"  << ACTION_DEBUG(probeResponse.body.msg_type&255)
                              << QString("0x%1").arg(probeResponse.body.msg_type,0,16);
#endif
            } // else qDebug() << "waitForReadyRead(100) timed out tm" << tm.elapsed();
        }
    }

    return rxdResp;
}



void    networkClient::haltNetworkClient(void)
{
    for( int penNum=0; penNum<MAX_SCREEN_PENS; penNum++ )
    {
        receivedPenCursor[penNum]->hide();
    }

    viewingReceivedStream = false;
    behindTheAnnotations  = NULL;

    disconnect(receiveSocket,SIGNAL(connected()),   this,SLOT(viewConnected()));
    disconnect(receiveSocket,SIGNAL(disconnected()),this,SLOT(viewDisconnected()));

    keepConnectionRunning = false;
    doNotExit             = false;
}


void    networkClient::readThread()
{
    qDebug() << "# networkClient task started.";

    connectedToHost = false;
    doNotExit       = true;

    do
    {
        // Wait until we connect (we don't unlock it here... if have new connection, the start will unlock it)
        keepConnectionRunning  = true;  // this will be cleared to command a disconnect

        qDebug() << "Wait for a start command...";
        blockRunStart->lock();
        blockRunStart->unlock(); // Don't want to get in the way of someone trying to stop us
        qDebug() << "Start command...";

        // negotiateBroadcastView() can take a couple of seconds...
        if( negotiateBroadcastView() )
        {
            // Preferred option: UDB broadcast (less load on host), but need to be on same router
            broadcastView = true;
        }
        else
        {
            // Fallback: the TCP view option
            broadcastView = false;
        }

        if( broadcastView )
        {
            qDebug() << "Using broadcast view.";
#if 0
            QMessageBox::information(NULL,"View connected","Broadcast view being displayed.");
#endif
            displayUDPBroadcastView();
        }
        else
        {
            qDebug() << "Using TCP connection";
#if 0
            QMessageBox::information(NULL,"View connected","TCP connection view being displayed.");
#endif
            if( receiveSocket == NULL )
            {
                receiveSocket         = new QTcpSocket;

                receiveSocket->setSocketOption(QAbstractSocket::LowDelayOption,1);
                connect(receiveSocket,SIGNAL(connected()),   this,SLOT(viewConnected()));
                connect(receiveSocket,SIGNAL(disconnected()),this,SLOT(viewDisconnected()));
            }

            displayTCPRemoteView();
        }

        qDebug() << "Wait to allow the mainthread to grab the lock...";

        QThread::msleep(500);

    } while( doNotExit );

    qDebug() << "# read thread exit...";
    if( receiveSocket != NULL ) delete receiveSocket;

    receiveSocket = NULL;
}



// /////////////
// Private stuff
// /////////////



void networkClient::displayTCPRemoteView(void)
{
    quint8      w[4];
    int         idx;
    int         netClientReceivedWords;
    int         lastAckedReceivedWords;

    netClientReceivedWords = 0;
    lastAckedReceivedWords = 0;
    idx                    = 0;

    qDebug() << "# Main thread: TCP connect to view host";

    receiveSocket->connectToHost(hostViewAddr,hostViewPort,QIODevice::ReadWrite);
    receiveSocket->setSocketOption(QAbstractSocket::LowDelayOption,1);

    if( ! keepConnectionRunning || ! receiveSocket->waitForConnected(400) )
    {
        qDebug() << "# Connected to host ="     << connectedToHost
                 << "keepConnectionRunning =" << keepConnectionRunning;
        return;
    }

    // While connected...
    while( keepConnectionRunning )
    {
        if( receiveSocket->bytesAvailable() >= 4 )
        {
            receiveSocket->read((char *)&w, 4);

            if( decryptor != NULL )
            {
                decryptor->decryptNextWord(idx,w);
            }

            applyReceivedTCPWord( w[3], w[2], w[1], w[0] );

            netClientReceivedWords ++;
#if 1
            if( (netClientReceivedWords - lastAckedReceivedWords) > 255 )
            {
                w[0] = ACTION_ACKNOWLEDGE;
                w[1] = ( netClientReceivedWords >> 24 ) & 255;
                w[2] = ( netClientReceivedWords >> 16 ) & 255;
                w[3] = ( netClientReceivedWords >>  8 ) & 255; // NB only sending 1 in 256, so divide by 256

                if( sizeof(w) == receiveSocket->write((char *)&w, sizeof(w)) )
                {
//                      qDebug() << "Acknowledge packets:" << netClientReceivedWords
//                               << "last Ack was"         << lastAckedReceivedWords;

                    lastAckedReceivedWords = netClientReceivedWords;
                }
                else qDebug() << "Failed to ack packet" << netClientReceivedWords;

                if( ! receiveSocket->waitForBytesWritten(100) )
                {
                    qDebug() << "Failed to ACK!";
                }
            }
#endif
        }
        else    // Line is quiet
        {
//                    qDebug() << "Quiet... # Rx-ed =" << netClientReceivedWords;

            receiveSocket->waitForReadyRead(400);   // Wait for up to 0.4 sec
        }
    }

    qDebug() << "# Disconnected.";

    receiveSocket->close();
    connectedToHost = false;
}



void networkClient::displayUDPBroadcastView(void)
{
    QByteArray         packetIn;
    udp_view_packet_t *packetData = (udp_view_packet_t *)packetIn.data();
    QHostAddress       senderAddress;
    quint16            senderPort;
    int                delaySinceRead;

    // handle connection in the same way as a TCP connection
    viewConnected();

    delaySinceRead = 0;

    while( keepConnectionRunning )
    {
        // The timeout here is to allow us to stop the thread relatively cleanly...
        if( broadcastSocket->waitForReadyRead(200) )
        {
            if( broadcastSocket->pendingDatagramSize() < MAX_ACCEPTED_DATAGRAM_SZ )
            {
                // Grow in buffer as necessary
                if( broadcastSocket->pendingDatagramSize() > packetIn.size() )
                {
                    packetIn.resize( broadcastSocket->pendingDatagramSize() );
                    packetData = (udp_view_packet_t *)packetIn.data(); // in case this moves the block
                }

                if( broadcastSocket->readDatagram(packetIn.data(), packetIn.size(),
                                                 &senderAddress,  &senderPort )  > 0 )
                {
                    applyReceivedAction(packetData->update_num,         packetData->body.data,
                                        packetIn.size()-sizeof(quint32) );
                }
            }
            else
            {
                qDebug() << "Unexpectedly large UDP broadcast packet:"
                         << broadcastSocket->pendingDatagramSize()
                         << "bytes.";

                // Kack monitor for lost host (other stuff could trigger us)
                delaySinceRead += 200;

                // Discard it
                broadcastSocket->readDatagram(packetIn.data(), packetIn.size(),
                                             &senderAddress,  &senderPort );
            }
        } // wait for ready read
    }

    broadcastSocket->close();
    delete broadcastSocket;
    broadcastSocket = NULL;

    qDebug() << "# Disconnected.";
    connectedToHost = false;
}



void    networkClient::applyReceivedTCPWord(quint8 byte1, quint8 byte2, quint8 byte3, quint8 byte4)
{
#ifdef RECREATE_STICKY_NOTES_LOCALLY
    stickyNote *sn;
#endif
    int                  ptr;
    int                  penNum;
    int                  sz;
    unsigned int         length, numRepeats;
#ifdef SHOW_RX_COUNT
    numBytesReceived ++;
    if( (numBytesReceived % 1000) == 0 ) qDebug() << "Received" << numBytesReceived << "words.";
#endif
    if( ! viewingReceivedStream )
    {
//        Commented out as get a lot of these on disconnect due to buffered data.
//        qDebug() << "applyReceivedAction called when not viewing received annotations.";
        return;
    }

    rxStream.push_back(byte4);
    rxStream.push_back(byte3);
    rxStream.push_back(byte2);
    rxStream.push_back(byte1);

    unsigned char *rxUnsignedStream = (unsigned char *)(rxStream.constData());

    // Ensure we have the full packet

    switch( rxUnsignedStream[0] )
    {
    case ACTION_SCREENSIZE:
        if( rxStream.size() < 8 )
        {
            // More to be read before we can use it...
            return;
        }
        break;

    case ACTION_BACKGROUND_PNG:
        length = (rxUnsignedStream[1]*256*256) + (rxUnsignedStream[2]*256) + (rxUnsignedStream[3]);
        if( rxStream.size() == 4 )
        {
            qDebug() << "ACTION_BACKGROUND_PNG started. Size =" << length;
            return;
        }

        if( rxStream.size() < (4+(int)length) )
        {
            // More to be read before we can use it...
            return;
        }
        break;

    case ACTION_BACKGROUND_JPG:
        length = (rxUnsignedStream[1]*256*256) + (rxUnsignedStream[2]*256) + (rxUnsignedStream[3]);
        if( rxStream.size() == 4 )
        {
            qDebug() << "ACTION_BACKGROUND_JPG started. Size =" << length;
            return;
        }

        if( rxStream.size() < (4+(int)length) )
        {
            // More to be read before we can use it...
            return;
        }
        break;

    case ACTION_BACKGROUND_COLOUR:
        if( rxStream.size() < 8 )
        {
            // More to be read before we can use it...
            return;
        }
        break;

    case ACTION_TRIANGLE_BRUSH:
        if( rxStream.size() < 8 )
        {
            // More to be read before we can use it...
            return;
        }
        break;

    case ACTION_BOX_BRUSH:
        if( rxStream.size() < 8 )
        {
            // More to be read before we can use it...
            return;
        }
        break;

    case ACTION_CIRCLE_BRUSH:
        if( rxStream.size() < 8 )
        {
            // More to be read before we can use it...
            return;
        }
        break;

    case ACTION_DOBRUSH:
        if( rxStream.size() < 8 )
        {
            // More to be read before we can use it...
            return;
        }
        break;

    case ACTION_PENPOSITIONS:

#ifdef SHOW_ALL_RECEIVED_MESSAGES
        if( rxStream.size() == 4 ) qDebug() << "pen positions";
#endif
        ptr = 4;
        for( penNum=0; penNum<MAX_SCREEN_PENS; penNum++ )
        {
            if( ((rxUnsignedStream[2]*256 + rxUnsignedStream[3]) & (1 << penNum)) )
            {
                if( rxStream.size() < (ptr+4) )
                {
                    // More to be read before we can use it...
                    return;
                }
            }
        }
        break;

    case ACTION_PENCOLOUR:
        if( rxStream.size() < 8 )
        {
            // More to be read before we can use it...
            return;
        }
        break;

    case ACTION_BACKGROUND_BA_COMPRESSED:
        // Compressed image data (slow update experimental fix)
        length = ((rxUnsignedStream[1]&0xF)*256*256) + (rxUnsignedStream[2]*256) + (rxUnsignedStream[3]);
        if( rxStream.size() == 4 )
        {
//            qDebug() << "BA Compressed tile received. length =" << length;
        }

        if( rxStream.size() < ((int)length+8) )
        {
            // More to be read before we can use it...
            return;
        }
        break;

    case ACTION_BG_PNG_TILE:
        // Background tile update (PNG image)
        length = (rxUnsignedStream[1]*256*256) + (rxUnsignedStream[2]*256) + (rxUnsignedStream[3]);

        if( rxStream.size() < ((int)length+8) )
        {
            // More to be read before we can use it...
            return;
        }
        break;

    case ACTION_BG_JPEG_TILE:
        // Background tile update (PNG image)
        length = ((rxUnsignedStream[1]&0xF)*256*256) + (rxUnsignedStream[2]*256) + (rxUnsignedStream[3]);

        sz = 1<<(rxUnsignedStream[1]>>4);

#ifdef SHOW_ALL_RECEIVED_MESSAGES
        if( rxStream.size() == 4 )
        {
            qDebug() << "Big tile tile received. JPEG length =" << length
                     << "small per big"   << sz
                     << "message length:" << ((int)length+8+(sz*sz/8));
        }
#endif
        if( rxStream.size() < ((int)length+8+(sz*sz/8)) )
        {
            // More to be read before we can use it...
            return;
        }
        break;

    case ACTION_BACKGROUND_SAME_TILE:

        numRepeats = (rxUnsignedStream[1]*256*256) + (rxUnsignedStream[2]*256) + (rxUnsignedStream[3]);
#ifdef SHOW_ALL_RECEIVED_MESSAGES
        if( rxStream.size() == 4 )
        {
            qDebug() << "PNG repeat tile received. numRepeats =" << numRepeats;
        }
#endif
        if( rxStream.size() < ((int)numRepeats*2+6) )
        {
            // More to be read before we can use it...
            return;
        }

        break;

    case ACTION_ADD_STICKY_NOTE:

        if( rxStream.size() < 8 )
        {
            // More to be read before we can use it...
            return;
        }
        break;

    case ACTION_MOVE_STICKY_NOTE:

        if( rxStream.size() < 8 )
        {
            // More to be read before we can use it...
            return;
        }
        break;

    case ACTION_PENS_PRESENT:

        // Just have 24 bits of pens from the initial message
        break;

    case ACTION_STARTDRAWING:
    case ACTION_STOPDRAWING:
    case ACTION_REDO:
    case ACTION_UNDO:
    case ACTION_BRUSHFROMLAST:
    case ACTION_PENWIDTH:
    case ACTION_PENTYPE:
    case ACTION_NEXTPAGE:
    case ACTION_PREVPAGE:
    case ACTION_GOTOPAGE:
    case ACTION_CLEAR_SCREEN:
    case ACTION_TIMEPOINT:
    case ACTION_CLIENT_PAGE_CHANGE:
    case ACTION_DELETE_STICKY_NOTE:
    case ACTION_ZOOM_STICKY_NOTE:
    case ACTION_SESSION_STATE:
    case ACTION_PEN_STATUS:
        break;

    default:
        qDebug() << "Unknown action:" << ACTION_DEBUG((int)(rxUnsignedStream[0])) << rxStream.toHex();
        QMessageBox::warning(NULL,tr("Bad Connection"),tr("Corrupted data received from host. Disconnecting."));
        emit remoteViewDisconnected();
        rxStream.clear();
        return;

        break;
    }

    // And perform the action
    applyReceivedAction(0,rxUnsignedStream,rxStream.length()); // NB This will probably break TCP...

    // Action complete:
    rxStream.clear();

    emit needsRepaint();

    return;
}

void  networkClient::applyReceivedAction(int tileUpdateNum, unsigned char *data, int len)
{
    int                  opt;
    QColor               col;
    QColor               backgroundColour;
    overlayWindow       *mw = (overlayWindow *)parentWindow;
    int                  ptr;
    int                  xLoc[MAX_SCREEN_PENS], yLoc[MAX_SCREEN_PENS];
    bool                 penPresent[MAX_SCREEN_PENS];                 // Moved here due to the compiler's moans.
    int                  x, y;
    int                  penNum;
    int                  sz;
    unsigned int         length, numRepeats, repeatTileIndex;

    switch( data[0] )
    {
    case ACTION_SCREENSIZE:

        w = (data[4]*256)|data[5];
        h = (data[6]*256)|data[7];

        if( w > 320 && h > 200 )
        {
            qDebug() << "ScreenSize:" << w << h << "widthInTiles =" << (w / BG_IMG_TILE_SIZE);

            adjustUpdateTileArraySizes();
            receivedAnnotations->setScreenSize(w,h);

            qDebug() << "completed receivedAnnotations->setScreenSize(w,h)";

            emit resolutionChanged(w,h); // Update image will be resized by this
        }
        else
        {
            qDebug() << "BAD ScreenSize received:" << w << h << "widthInTiles =" << (w / BG_IMG_TILE_SIZE);

            QMessageBox::warning(NULL,tr("Bad Connection"),tr("Corrupted data received from host. Disconnecting."));
            emit remoteViewDisconnected();
        }

        break;

    case ACTION_BACKGROUND_PNG:
        length = (data[1]*256*256) + (data[2]*256) + (data[3]);
        qDebug() << "Background PNG fully received.";
        applyReceivedBackgroundPNGImage(tileUpdateNum,data+4,length); // length = bottom 24 bits
        break;

    case ACTION_BACKGROUND_JPG:
        length = (data[1]*256*256) + (data[2]*256) + (data[3]);
        qDebug() << "Background JPG fully received.";
        applyReceivedBackgroundJPGImage(tileUpdateNum,data+4,length); // length = bottom 24 bits
        break;

    case ACTION_BACKGROUND_COLOUR:

        backgroundColour = QColor(data[4],data[5],data[6],data[7]);
        qDebug() << "Background colour:" << data[4] << data[5]
                                         << data[6] << data[7];
        // Need a different code for replay (we don't want local rebuild of current image
        receivedAnnotations->setBackground(backgroundColour);
        break;

    case ACTION_STARTDRAWING:
        qDebug() << "Start drawing pen" << (int)(data[3]);
        receivedAnnotations->startPenDraw(data[3]);
        break;

    case ACTION_STOPDRAWING:
        qDebug() << "Stop drawing pen" << (int)(data[3]);
        receivedAnnotations->stopPenDraw(data[3]);
        break;

    case ACTION_REDO:
        qDebug() << "Redo pen" << (int)(data[3]);
        receivedAnnotations->redoPenAction(data[3]);
        break;

    case ACTION_UNDO:
        qDebug() << "Undo pen" << (int)(data[3]);
        receivedAnnotations->undoPenAction(data[3]);
        break;

    case ACTION_BRUSHFROMLAST:
        qDebug() << "BrushFromLast pen" << (int)(data[3]);
        receivedAnnotations->brushFromLast(data[3]);
        break;

    case ACTION_TRIANGLE_BRUSH:
        qDebug() << "TriangleBrush pen" << (int)(data[3]);
        receivedAnnotations->explicitBrush(data[3],SHAPE_TRIANGLE,
                                         (data[4]*256)+data[5],
                                         (data[6]*256)+data[7]);
        break;

    case ACTION_BOX_BRUSH:
        qDebug() << "BoxBrush pen" << (int)(data[3]);
        receivedAnnotations->explicitBrush(data[3],SHAPE_BOX,
                                         (data[4]*256)+data[5],
                                         (data[6]*256)+data[7]);
        break;

    case ACTION_CIRCLE_BRUSH:
        qDebug() << "CircleBrush pen" << (int)(data[3]);
        receivedAnnotations->explicitBrush(data[3],SHAPE_CIRCLE,
                                         (data[4]*256)+data[5],
                                         (data[6]*256)+data[7]);
        break;

    case ACTION_DOBRUSH:
        qDebug() << "DoBrush pen" << (int)(data[3]);
        receivedAnnotations->CurrentBrushAction(data[3],
                                           (data[4]*256)+data[5],
                                           (data[6]*256)+data[7]);
        break;

    case ACTION_PENPOSITIONS:

        ptr = 4;
        for( penNum=0; penNum<MAX_SCREEN_PENS; penNum++ )
        {
            // Don't do the extra byte unless it's required
            if( ((data[2]*256 + data[3]) & (1 << penNum)) )
            {
                xLoc[penNum] = data[ptr+0]*256 + data[ptr+1];
                yLoc[penNum] = data[ptr+2]*256 + data[ptr+3];

                penPresent[penNum] = true;

                ptr += 4;
            }
            else
            {
                penPresent[penNum] = false;
            }
        }

        drawLocalCopyOfRemoteCursors( penPresent, xLoc, yLoc );
        receivedAnnotations->penPositions( penPresent, xLoc, yLoc );
        break;

    case ACTION_PENCOLOUR:
        col    = QColor(data[4],data[5],data[6],data[7]);
        penNum = data[3];   // Legibility
        receivedPenCursor[penNum]->updatePenColour(col);
        qDebug() << " pen " << penNum << " colour " << col.name() << " ";
        receivedAnnotations->setPenColour(penNum,col);
        break;

    case ACTION_PENWIDTH:

        qDebug() << " pen " << ((int)(data[3])) << " width " << ((int)(data[2])) << " ";
        penNum = data[3];   // Legibility
        receivedPenCursor[penNum]->updatePenSize(data[2]);
        receivedAnnotations->setPenThickness(penNum,data[2]);
        break;

    case ACTION_PENTYPE:

        qDebug() << " pen " << ((int)(data[3])) << " type " << ((int)(data[2])) << " ";
        switch( rxStream[2] )
        {
        case TYPE_DRAW:        receivedAnnotations->penTypeDraw( data[3] );
                               break;
        case TYPE_HIGHLIGHTER: receivedAnnotations->penTypeHighlighter( data[3] );
                               break;
        case TYPE_ERASER:      receivedAnnotations->penTypeEraser( data[3] );
                               break;
        }
        break;

    case ACTION_NEXTPAGE:

        // TODO: NextPage and replace with clear page - expect the new page to be transmitted.
        qDebug() << "TODO: remove NextPage and replace with clear page (remote sessions retransmit new pages)";
        receivedAnnotations->nextPage();
        break;

    case ACTION_PREVPAGE:

        // TODO: PrevPage and replace with clear page - expect the new page to be transmitted.
        qDebug() << "TODO: PrevPage and replace with clear page (remote sessions retransmit new pages)";
        receivedAnnotations->previousPage();
        break;

    case ACTION_GOTOPAGE:

        // TODO: GotoPage and replace with clear page - expect the new page to be transmitted.
        qDebug() << "TODO: GotoPage and replace with clear page (remote sessions retransmit new pages)" << ((data[1]*256*256)+(data[2]*256)+data[3]);
        receivedAnnotations->gotoPage((data[1]*256*256)+(data[2]*256)+data[3]);
        break;

    case ACTION_CLEAR_SCREEN:

        // TODO: GotoPage and replace with clear page - expect the new page to be transmitted.
        qDebug() << "Clear page";
        receivedAnnotations->clearOverlay();
        break;

    case ACTION_BACKGROUND_BA_COMPRESSED:
        // Compressed image data (slow update experimental fix)
        length = ((data[1]&0xF)*256*256) + (data[2]*256) + (data[3]);

#ifdef SHOW_ALL_RECEIVED_MESSAGES
        qDebug() << "Background tile update @" << data[4]*256 + data[5]
                                               << data[6]*256 + data[7];
#endif
        sz = data[1]>>4;
        x  = data[4]*256 + data[5];
        y  = data[6]*256 + data[7];
#if 1
        if( len<(8+length) )
            qDebug() << "Insufficient space for compressed data: need"
                     << (8+length) << "got" << len;
        else
#endif
        updateBACompressedBackgroundImageTile(tileUpdateNum, x,y, sz, length, data+8 );
        break;

    case ACTION_BG_PNG_TILE:
        // Background tile update (PNG image)
        length = (data[1]*256*256) + (data[2]*256) + (data[3]);

#ifdef SHOW_ALL_RECEIVED_MESSAGES
        qDebug() << "Background tile update @" << data[4]*256 + data[5]
                                               << data[6]*256 + data[7]
                 << "size =" << length;
#endif
        x = data[4]*256 + data[5];
        y = data[6]*256 + data[7];
        updateBackgroundImageTile(tileUpdateNum, x,y, length, data+8 );
        break;

    case ACTION_BG_JPEG_TILE:
        // Background tile update (PNG image)
        if( len<8 )
        {
            qDebug() << "ACTION_BG_JPEG_TILE: too short. Len=" << len;
            return;
        }
        length = ((data[1]&0xF)*256*256) + (data[2]*256) + (data[3]);

        sz = 1<<(data[1]>>4);

        x = data[4]*256 + data[5];
        y = data[6]*256 + data[7];
        if( len<(length+8) )
        {
            qDebug() << "ACTION_BG_JPEG_TILE: too short." << "Required"
                     <<  "Len=" << (length+8) << "but got" << len;
        }
        updateJPEGCompressedBackgroundImageTile(tileUpdateNum, x,y, length,sz, data+8 );
        break;

    case ACTION_BACKGROUND_SAME_TILE:

        numRepeats      = (data[1]*256*256) + (data[2]*256) + (data[3]);
        repeatTileIndex = (data[4]*256) + (data[5]);

        repeatBackgroundImageTile(numRepeats, repeatTileIndex, data+4);
        break;

    case ACTION_TIMEPOINT:

        qDebug() << "TimePoint" << ((rxStream[1]<<16)+(rxStream[2]<<8)+rxStream[3]);
        break;

    case ACTION_CLIENT_PAGE_CHANGE:

        qDebug() << "Client page change.";
        break;

    case ACTION_ADD_STICKY_NOTE:

        // penNum     = data[3]
        // stickySize = 256*data[1] + data[2]
        length = 256*data[1] + data[2];
        x = data[4]*256 + data[5];
        y = data[6]*256 + data[7];
#ifdef RECREATE_STICKY_NOTES_LOCALLY
        sn = new stickyNote(length, length, data[3], Qt::yellow, parentWindow);
        sn->move(x,y);
        sn->show();

        notes.append(sn);

        // Bump our pens above everything to make 'em above the new note
        for(int pen=0; pen<MAX_SCREEN_PENS; pen++)
        {
            if( receivedPenCursor[pen]->isVisible() )
            {
                receivedPenCursor[pen]->raise();
            }
        }
#else
        qDebug() << "Add sticky notes by echo-ed screen";
#endif
        break;

    case ACTION_MOVE_STICKY_NOTE:

        if( rxStream.size() < 8 )
        {
            // More to be read before we can use it...
            return;
        }
        // penNum     = data[3]
        // stickyNum = 256*data[1] + data[2]
        ptr = (256*data[1] + data[2]) & 32767;
        x = data[4]*256 + data[5];
        y = data[6]*256 + data[7];
#ifdef RECREATE_STICKY_NOTES_LOCALLY
        notes[ptr]->move(x,y);
#else
        qDebug() << "Move sticky notes by echo-ed screen";
#endif
        break;

    case ACTION_DELETE_STICKY_NOTE:

        // Sticky number
        ptr = (256*data[1] + data[2]) & 32767;
#ifdef RECREATE_STICKY_NOTES_LOCALLY
        // Remove the sticky
        notes[ptr]->hide();
        notes.removeAt(ptr);
#else
        qDebug() << "Delete sticky notes by echo-ed screen";
#endif

        break;

    case ACTION_ZOOM_STICKY_NOTE:

        // Sticky number
        ptr = (256*data[1] + data[2]) & 32767;

        // Remove the sticky
        qDebug() << "TODO: Sticky zoom. For sticky" << ptr;

        break;

    case ACTION_SESSION_STATE:

        opt = (256*data[1] + data[2]) & 32767;
        qDebug() << "session state =" << opt;

        mw->recordRemoteSessionType(opt);

        break;

    case ACTION_PEN_STATUS:

        opt = (256*data[1] + data[2]) & 32767;
        qDebug() << "pen status =" << opt;

        mw->recordReceivedPenRole(opt,data[3]);

        break;

    case ACTION_VIEW_BROADCAST_ACK:

        qDebug() << "Remaining join ACK";
        break;

    case ACTION_PENS_PRESENT:

        checkPenPresent(data+1);
        break;

    default:
        qDebug() << "Unknown action:" << ACTION_DEBUG((int)(data[0])) << ((int)(data[0])) << "len" << len;
        QMessageBox::warning(NULL,tr("Bad Connection"),tr("Corrupted data received from host. Disconnecting."));
        emit remoteViewDisconnected();

        break;
    }

    emit needsRepaint();

    return;
}



void networkClient::enablePenReconnect(bool enable)
{
    reconnectRemotePenOnDisconnect = enable;
}



void networkClient::checkPenPresent(unsigned char *data)
{
    // Don't do this in view only mode
    if( viewOnlyMode ) return;

    if( countDownBeforeStartChecking > 0 )
    {
        countDownBeforeStartChecking --;
        return;
    }

    // Check up to 24 pens (the remaining 3 bytes)

    bool present;
    bool attemptReconnection = false;

    for( int penNumber = 0; penNumber<MAX_SCREEN_PENS; penNumber ++ )
    {
        int byte = 2 - (penNumber>>8);

        present = ( data[byte] & (1<<(penNumber&7)) ) != 0;

        if( penPresent[penNumber] != present )
        {
            if( present ) qDebug() << "Pen" << penNumber << "joined.";
            else          qDebug() << "Pen" << penNumber << "lost.";

            // Note that we only trigger lost if we had had a we were found before.
            // This allows for both starting up, and prevents multiple detections of loss.
            if( penNumber == penOnRemoteSystem && ! present && penPresent[penNumber] )
            {
                qDebug() << "THIS DEVICES PEN WAS LOST!";

                if( reconnectRemotePenOnDisconnect ) attemptReconnection = true;
            }
        }

        penPresent[penNumber] = present;
    }

    if( attemptReconnection )
    {
        emit remotePenDisconnected();
    }
}



void networkClient::applyReceivedBackgroundPNGImage(int updateNum, unsigned char *data, int length)
{
    QImage replayBackground;

    // TODO: generate a mask for these full screen tiles. Not currently use, so ignore ATM

    try
    {
        replayBackground.loadFromData(data,length,"PNG");

        qDebug() << "Apply received background image. Length =" << length;

        QPainter paint(behindTheAnnotations);
        paint.setCompositionMode(QPainter::CompositionMode_Source);
        paint.drawImage(0,0,replayBackground);
        paint.end();
    }
    catch( std::bad_alloc &a )
    {
        qDebug() << "Failed to apply full screen image due to memory allocation failure:" << a.what();
    }
}


void networkClient::applyReceivedBackgroundJPGImage(int updateNum, unsigned char *data, int length)
{
    QImage replayBackground;

    // TODO: generate a mask for these full screen tiles. Not currently use, so ignore ATM

    try
    {
        replayBackground.loadFromData(data,length,"JPG");

        qDebug() << "Apply received background image. Length =" << length;

        QPainter paint(behindTheAnnotations);
        paint.setCompositionMode(QPainter::CompositionMode_Source);
        paint.drawImage(0,0,replayBackground);
        paint.end();
    }
    catch( std::bad_alloc &a )
    {
        qDebug() << "Failed to apply full screen image due to memory allocation failure:" << a.what();
    }
}


void networkClient::drawLocalCopyOfRemoteCursors( bool *penPresent, int *xLoc, int *yLoc )
{
    for( int penNum=0; penNum<MAX_SCREEN_PENS; penNum++ )
    {
        // Draw the local pen in overlayWindow using local data so it doesn't feel to laggy to
        // the user. It wont pain anyone else as they're not in the control loop.
        if( penNum != localPenNum )
        {
            if( ! penPresent[penNum] )
            {
                emit moveReceivedPen(0,0,0, false);
            }
            else
            {
                emit moveReceivedPen(penNum, xLoc[penNum],yLoc[penNum], true);
            }
        }
    }
}



void networkClient::updateBackgroundImageTile(int updateNum, int x, int y, int length, unsigned char *data)
{
    // Create the update image (unpack the received data)
    updateTile->loadFromData(data,length,"PNG");

    updateTileUpdateNum = updateNum;
    updateTileWidth     = updateTile->width();
    updateTileHeight    = updateTile->height();

    // Only apply this tile if it is newer than the latest small tile). It should always be.
    if( updateNum > latestSmallTileUpdateNum(x/updateTileWidth, y/updateTileHeight) )
    {
        return;
    }

    // Update the stored update number for these smallest, pixel perfect tiles

    storeSmallTileUpdateNum(x/updateTileWidth,y/updateTileHeight,updateNum);

    // And draw it onto the background image

    QPainter paint(behindTheAnnotations);
    paint.drawImage(QPoint(x,y), *updateTile);
    paint.end();
}



void networkClient::updateBACompressedBackgroundImageTile(int updateNum, int x, int y, int sz, int length, quint8 *data)
{
    updateTileUpdateNum = updateNum;
    updateTileWidth     = 1<<sz;
    updateTileHeight    = 1<<sz;

    // Only apply this tile if it is newer
    if( updateNum < latestSmallTileUpdateNum(x/updateTileWidth, y/updateTileHeight) )
    {
        return;
    }

//    qDebug() << "update small tile @" << x << y << "with update num" << updateNum;

    // Update the stored update number for these smallest, pixel perfect tiles

    storeSmallTileUpdateNum(x/updateTileWidth,y/updateTileHeight,updateNum);

    // Extract pixel data into the receivedImageData QByteArray

    try
    {
        delete updateTile;
        delete receivedImageData;

        // Leave a copy hanging around for use in repeat tiles
        receivedImageData = new QByteArray( qUncompress(data,length) );

        updateTile = new QImage((unsigned char *)(receivedImageData->data()), updateTileWidth, updateTileHeight, QImage::Format_RGB888);

        // And draw it onto the background image
        if( x>=0   &&   y>=0   &&
            x<behindTheAnnotations->width() && y<behindTheAnnotations->height() )
        {
            QPainter paint(behindTheAnnotations);
            QPoint   pos(x,y);
            paint.drawImage(pos, *updateTile);
            paint.end();
        }
    }
    catch( std::bad_alloc &a )
    {
        qDebug() << "Failed to apply full screen image due to memory allocation failure:" << a.what();
    }
}


//void networkClient::updateBACompressedBackgroundImageTileWithRepeats(int updateNum, int x, int y, int sz,
//                                                                     int tileDataLength, int numRepeats,
//                                                                     quint8 *repeatData, quint8 *imageData)
//{
//    updateBACompressedBackgroundImageTile(updateNum,x,y,sz,tileDataLength,imageData);
//    repeatBackgroundImageTile(numRepeats, , repeatData);
//}

// Big tiles
void networkClient::updateJPEGCompressedBackgroundImageTile(int updateNum, int drawX, int drawY, int length,
                                                            int newNumBigTilesPerSmall,
                                                            quint8 *data)
{
    // Create the update image (unpack the received data that follows the mask)
    QImage jpegTile;
    if( ! jpegTile.loadFromData(data,length,"JPEG") )
    {
        qDebug() << "Failed to expand BIG jpeg tile! Data len:" << length <<
                    "data:" << data[0] << data[1] << data[2] << data[3];
        return;
    }

    int jpegTileWidth  = jpegTile.width();
//    int jpegTileHeight = jpegTile.height();

    if( updateTileWidth*numBigTilesPerSmall != jpegTileWidth )
    {
        qDebug() << "Sanifty check failed: received BIG jpeg tile" << jpegTile.width()
                 << "wide, but scaled tile size =" << numBigTilesPerSmall << "*"
                 << updateTileWidth << "i.e." << updateTileWidth*numBigTilesPerSmall;

        return;
    }

    numBigTilesPerSmall = newNumBigTilesPerSmall;

    int xInSmallTiles = numBigTilesPerSmall * drawX / jpegTileWidth;
    int yInSmallTiles = numBigTilesPerSmall * drawY / jpegTileWidth;

    if( ! jpegTile.hasAlphaChannel() )
    {
        jpegTile = jpegTile.convertToFormat(QImage::Format_ARGB32);
    }

    // Mask out the subtiles that are more up to date (they have higher resolution)
    QImage mask(updateTileWidth,updateTileWidth,QImage::Format_ARGB32);
    mask.fill(Qt::transparent);

    QPainter paintUpdate(&jpegTile);

    // Calculate the paint mask
    paintUpdate.setCompositionMode(QPainter::CompositionMode_Source);
    QPoint   pos;
    int      numTransp = 0;
    for(int y=0; y<numBigTilesPerSmall; y++)
    {
        int     yLine = y + yInSmallTiles;

        for(int x=0; x<numBigTilesPerSmall; x++)
        {
            // If this is not newer, paint update as transparent to leave the current image data
            if( updateNum <= latestSmallTileUpdateNum(x+xInSmallTiles,yLine) )
            {
                numTransp ++;

                pos.setX(x*updateTileWidth);
                pos.setY(y*updateTileWidth);
                paintUpdate.drawImage(pos, mask);
            }
        }
    }

    paintUpdate.end();

    if( numTransp > 0 )
    {
//        qDebug() << "Updated" << (numBigTilesPerSmall*numBigTilesPerSmall - numTransp)
//                 << "out of"  << (numBigTilesPerSmall*numBigTilesPerSmall);
    }

    // Store big tile updateNumber
    storeBigTileUpdateNum(drawX / jpegTileWidth, drawY / jpegTileWidth, updateNum);
    numberOfBigTilesReceived ++;

    // And draw it onto the background image

    if( haveNotEmmitedConnected )
    {
        if( numberOfBigTilesReceived == 1 )
        {
            // TODO: Draw the image everuwhere (so we have a non empty screen to start with)
        }
        else if( numberOfBigTilesReceived > bigTileUpdateNum.size() )
        {
            haveNotEmmitedConnected = false;
            emit remoteViewAvailable();
        }
    }

    QPainter paint(behindTheAnnotations);
    pos.setX(drawX);
    pos.setY(drawY);
    paint.drawImage(pos, jpegTile);
    paint.end();
}


// Uses the globals w & h (width and height) in pixels.
// Small tiles are 16x16 (updateTileWidth^2), big are 8x8 (numBigTilesPerSmall^2) of these.
void networkClient::adjustUpdateTileArraySizes(void)
{
    int numRequiredSmallHTiles = ((w+updateTileWidth+1)/updateTileWidth);
    int numRequiredSmallVTiles = ((h+updateTileHeight+1)/updateTileHeight);

    widthInSmallTiles = numRequiredSmallHTiles;

    smallTileUpdateNum.resize(numRequiredSmallHTiles*numRequiredSmallVTiles);
    smallTileUpdateNum.fill(-1);

    int numRequiredBigHTiles = ((numRequiredSmallHTiles+numBigTilesPerSmall+1)/numBigTilesPerSmall);
    int numRequiredBigVTiles = ((numRequiredSmallVTiles+numBigTilesPerSmall+1)/numBigTilesPerSmall);

    widthInBigTiles = numRequiredBigHTiles;

    bigTileUpdateNum.resize(numRequiredBigHTiles*numRequiredBigVTiles);
    bigTileUpdateNum.fill(-1);
}


void networkClient::storeSmallTileUpdateNum(int xInSmallTiles, int yInSmallTiles, int updateNum)
{
    lastSmallTileIndex                     = xInSmallTiles + yInSmallTiles*widthInSmallTiles;
    smallTileUpdateNum[lastSmallTileIndex] = updateNum;
}


void networkClient::storeBigTileUpdateNum(int xInBigTiles,int yInBigTiles, int updateNum)
{
    bigTileUpdateNum[xInBigTiles + yInBigTiles*widthInBigTiles] = updateNum;
}


int networkClient::latestSmallTileUpdateNum(int xInSmallTiles,int yInSmallTiles)
{
    return smallTileUpdateNum[xInSmallTiles + yInSmallTiles*widthInSmallTiles];
}


int networkClient::latestBigTileUpdateNum(int xInBigTiles,int yInBigTiles)
{
    return bigTileUpdateNum[xInBigTiles + yInBigTiles*widthInBigTiles];
}


// NB This is only for small tiles, and so updateTileWidth is a small tile width...
void networkClient::repeatBackgroundImageTile(int numRepeats, int repeatTileIndex, unsigned char *data)
{
    int  x, y;
    int  tileNumber;

    // Yuck, but it's late and I'm shattered...
    int  widthInTiles = (w / updateTileWidth) / numBigTilesPerSmall;

    if( (widthInTiles * numBigTilesPerSmall) != (w / updateTileWidth) )
    {
        widthInTiles ++;
    }
    widthInTiles = widthInTiles * numBigTilesPerSmall;


//    QString listStr;                          // Debug
#ifdef SHOW_TILE_DEBUG
    if( repeatTileIndex != lastSmallTileIndex ) {
        qDebug() << "Repeat tile index mismatch (probably lost base tile). Last:" << lastSmallTileIndex
                 << "repeat:" << repeatTileIndex;
        return;
    }
#endif
    QPainter paint(behindTheAnnotations);

    // Each repeat takes 2 bytes (a tile number). [NB this gives up to 2* 3840x2146 using 16x16 tiles]

    for(int repeatNumber=0; repeatNumber<numRepeats; repeatNumber ++)
    {
        tileNumber = data[0]*256 + data[1];
        data      += 2;

        x = (tileNumber % widthInTiles);
        y = (tileNumber / widthInTiles);

        if( updateTileUpdateNum > latestSmallTileUpdateNum(x,y) )
        {
            storeSmallTileUpdateNum(x,y,updateTileUpdateNum);

            x = (tileNumber % widthInTiles) * updateTileWidth;
            y = (tileNumber / widthInTiles) * updateTileHeight;

            paint.drawImage(QPoint(x,y), *updateTile);
        }
    }

    paint.end();

//    qDebug() << "Update repeated tile for" << numRepeats;
}
