#include "viewBroadcaster.h"
#include "overlay.h"

#ifdef __GNUC__
#include <cmath>
#endif

// Debug
#define DEBUG_COUNT
// #define DEBUG_START_STOP


#ifdef DEBUG_COUNT
static int  num_zero_len_tiles = 0;
#endif
#ifdef DEBUG_START_STOP
static bool sent_data_last_time = false;
#endif

viewBroadcaster::viewBroadcaster(screenGrabber *scnGrab, bool *penActiveArray, QObject *parent) : QThread(parent)
{
    screenGrabberTask       = scnGrab;
    penIsActive             = penActiveArray;
    lowDefRequired          = false;
    currentBigTileIndex     = 0;
    currentSmallTileIndex   = 0;
    pause                   = new QMutex;
    sendingFullscreenUpdate = false;
    fullscreenUpdateTileNum = 0;
    dataRateBitPerSec       = 500000; // 500Kbit/Sec
    currentTxRateKbs        = 500000;
}


viewBroadcaster::~viewBroadcaster()
{
    ;
}


void  viewBroadcaster::setAverageDataRate(int newRateBitPerSec)
{
    dataRateBitPerSec = newRateBitPerSec;
}



int viewBroadcaster::generateRepeatsMessage(QByteArray &message, int &updateNum)
{
    // Construct a repeat message
    int  numRepeatsInMessage = 0;
    message.resize(10); // Insert seq_num + tile being repeated + space at the start for the length

//        QString repeatsDbg = QString("Last %1 repeated at: ").arg(currentTileIndex);

    int firstRepeatTileNum = repeatList[0];

    do
    {
        if( repeatList[0]<0 )
        {
            //qDebug() << "RepeatList entry" << repeatList[0] << "less than zero!";
            repeatList[0] = 0; // Hack
        }
        else if( repeatList[0]>=lastSentSmallUpdateIndex.size() )
        {
            if( (repeatList[0] % 100) == 0 )
            {
                //qDebug() << "RepeatList entry" << repeatList[0]
                         //<< "larger than array:" << lastSentSmallUpdateIndex.size();
            }

            int oldSize = lastSentSmallUpdateIndex.size();
            lastSentSmallUpdateIndex.resize(repeatList[0]+1);
            numResendsSmallUpdateIndex.resize(repeatList[0]+1);
            for(int i=oldSize; i<lastSentSmallUpdateIndex.size(); i++) lastSentSmallUpdateIndex[i] = 0;
        }

        if( /* repeatList[0]                           != currentSmallTileIndex   && */
            lastSentSmallUpdateIndex[repeatList[0]] != repeatListUpdateNum[0]    )
        {
            numRepeatsInMessage ++;

            message.append( ( repeatList[0] >> 8 ) & 255 );
            message.append( ( repeatList[0]      ) & 255 );

//                repeatsDbg += QString("%1 ").arg(repeatList[0]);

            lastSentSmallUpdateIndex[repeatList[0]] = repeatListUpdateNum[0];
        }
//        else
//        {
//            qDebug() << "repeatList[0]" << repeatList[0] << "currentSmallTileIndex" << currentSmallTileIndex;
//            qDebug() << "lastSentSmallUpdateIndex[repeatList[0]]" << lastSentSmallUpdateIndex[repeatList[0]] << "repeatListUpdateNum[0]" << repeatListUpdateNum[0];
//        }

        repeatListUpdateNum.remove(0);
        repeatList.remove(0);
    }
    while( repeatList.size()  >  0                          &&
           message.size()    <= (REPEAT_MESSAGE_MAX_SIZE - 2) );

//    qDebug() << "Send repeats message: with" << numRepeatsInMessage
//             << "repeats. And" << repeatList.length() << "still to send. "
//             << "First tile:" << firstRepeatTileNum;

//    repeatList.squeeze();
//    repeatListUpdateNum.squeeze();

//        qDebug() << repeatsDbg;

    char *data_ptr = message.data();

    data_ptr[0] = (lastUpdateNumber     ) & 255;
    data_ptr[1] = (lastUpdateNumber >>  8) & 255;
    data_ptr[2] = (lastUpdateNumber >> 16) & 255;
    data_ptr[3] = (lastUpdateNumber >> 24) & 255;

    updateNum = lastUpdateNumber;

    data_ptr[4] = ACTION_BACKGROUND_SAME_TILE;
    data_ptr[5] = ( numRepeatsInMessage >> 16 ) & 255;
    data_ptr[6] = ( numRepeatsInMessage >>  8 ) & 255;
    data_ptr[7] = ( numRepeatsInMessage       ) & 255;

    data_ptr[8] = ( lastTileIndex >>  8 ) & 255;
    data_ptr[9] = ( lastTileIndex       ) & 255;

    return numRepeatsInMessage;
}


bool viewBroadcaster::generateSmallTileMessages(QByteArray &message, int &updateNum)
{
    // Recover next tile with list of repeats
    // Look for a tile that is out of date.
    bool    changeFound;
    int     startTileIndex = currentSmallTileIndex;

#ifdef DEBUG_COUNT
    if( currentSmallTileIndex == 0 && num_zero_len_tiles > 0 )
    {
        qDebug() << "Broadcast: Last small tile scan resulted in" << num_zero_len_tiles
                 << "zero length tiles (8160)";

        num_zero_len_tiles = 0;
    }
#endif
    // Check for out of date tiles. The "first transmit" list.
    do
    {
        updateNum = screenGrabberTask->nextSmallIndexesUpdateNum(currentSmallTileIndex);

        if( currentSmallTileIndex >= lastSentSmallUpdateIndex.size() )
        {
//            qDebug() << "Expand lastSentSmallUpdateIndex to" << (currentSmallTileIndex+1);

            lastSentSmallUpdateIndex.resize(currentSmallTileIndex+1);
            numResendsSmallUpdateIndex.resize(currentSmallTileIndex+1);
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
    message.resize(12); // Insert space at the start for: seq_num, type & length, coords

    lastSentSmallUpdateIndex[currentSmallTileIndex]   = updateNum;
    numResendsSmallUpdateIndex[currentSmallTileIndex] = 1;

    // We have a pending send tile
    QByteArray *blob = screenGrabberTask->grabSmallTileData(currentSmallTileIndex,updateNum);
    message.append( blob->constData(), blob->length() );

    if( message.size() < 13 )
    {
#ifdef DEBUG_COUNT
        if( screenGrabberTask->grabSmallTileData(currentSmallTileIndex,updateNum)->size() == 0 )
        {
            num_zero_len_tiles ++;
        }
        else
        {
#endif
//            qDebug() << "Broadcast: Received a bad data size for small tile"
//                     << currentSmallTileIndex << "Sz ="
//                     << screenGrabberTask->grabSmallTileData(currentSmallTileIndex,updateNum)->size();
#ifdef DEBUG_COUNT
        }
#endif
        // Nothing to send

        return false;
    }
#if 0
    unsigned char *dbg_ptr = (unsigned char *)(blob->data());
    int endPt = blob->length()-8;
    qDebug() << "BA len" << blob->length()
             << QString("start: %1%2%3%4%5%6%7%8 end %9%10%11%12%13%14%15%16")
                .arg(dbg_ptr[0],2,16).arg(dbg_ptr[1],2,16)
                .arg(dbg_ptr[2],2,16).arg(dbg_ptr[3],2,16)
                .arg(dbg_ptr[4],2,16).arg(dbg_ptr[5],2,16)
                .arg(dbg_ptr[6],2,16).arg(dbg_ptr[7],2,16)
                .arg(dbg_ptr[endPt],2,16).arg(dbg_ptr[endPt+1],2,16)
                .arg(dbg_ptr[endPt+2],2,16).arg(dbg_ptr[endPt+3],2,16)
                .arg(dbg_ptr[endPt+4],2,16).arg(dbg_ptr[endPt+5],2,16)
                .arg(dbg_ptr[endPt+6],2,16).arg(dbg_ptr[endPt+7],2,16);
#endif
    // TODO: tell screenGrabber that we're no longer using *blob (shouldn't really be an issue, but safer)

    screenGrabberTask->getRepeatsAndUpdateIndex(currentSmallTileIndex, repeatList, repeatListUpdateNum);

    int imageSize = message.size() - 12;

    char *data_ptr = message.data();

    data_ptr[0] = (updateNum      ) & 255;
    data_ptr[1] = (updateNum >>  8) & 255;
    data_ptr[2] = (updateNum >> 16) & 255;
    data_ptr[3] = (updateNum >> 24) & 255;

    lastUpdateNumber = updateNum;

#ifdef PNG_COMPRESS_TILES
    data_ptr[4] = ACTION_BG_PNG_TILE;
    data_ptr[5] = ( imageSize >> 16 );
#else
    data_ptr[4] = ACTION_BACKGROUND_BA_COMPRESSED;
    data_ptr[5] = ( imageSize >> 16 ) & 0x0F | (BG_IMG_TILE_POW<<4);
#endif
    data_ptr[6] = ( imageSize >>  8 ) & 0xFF;
    data_ptr[7] = ( imageSize       ) & 0xFF;

    int x, y;
    screenGrabberTask->screenCoordsOfSmallTile(currentSmallTileIndex, x, y);

    data_ptr[8]  = (x >> 8) & 255;
    data_ptr[9]  =  x       & 255;
    data_ptr[10] = (y >> 8) & 255;
    data_ptr[11] =  y       & 255;

    lastTileIndex = currentSmallTileIndex;

    return true;
}

int viewBroadcaster::numOutOfDateBigTiles(void)
{
    int  chkIndex = 0;
    int  outOfDateCount = 0;
    int  updateNum;
    int  x, y;

    do
    {
        screenGrabberTask->nextBigTileIndexAndCoords(chkIndex,updateNum,x,y);

        // Allow for changing array sizes
        if( chkIndex >= lastSentBigUpdateIndex.size() )
        {
//            qDebug() << "Expand lastSentBigUpdateIndex to" << (currentBigTileIndex+1);

            int lastSize = lastSentBigUpdateIndex.size();

            lastSentBigUpdateIndex.resize(chkIndex+1);

            for(int i=lastSize; i<=currentBigTileIndex; i++)
            {
                lastSentBigUpdateIndex[i] = -1;
            }
        }

        if( updateNum != lastSentBigUpdateIndex[chkIndex] ) outOfDateCount ++;
    }
    while( chkIndex != 0 );

//    if( outOfDateCount != 0 ) qDebug() << outOfDateCount << "Out of date big tiles.";

    return outOfDateCount;
}


bool viewBroadcaster::generateBigTileMessages(QByteArray &message, int &updateNumber)
{
    int  xBig, yBig;

    do
    {
        screenGrabberTask->nextBigTileIndexAndCoords(currentBigTileIndex, updateNumber, xBig, yBig);

        // Allow for changing array sizes
        if( currentBigTileIndex >= lastSentBigUpdateIndex.size() )
        {
    //        qDebug() << "Expand lastSentBigUpdateIndex to" << (currentBigTileIndex+1);

            int lastSize = lastSentBigUpdateIndex.size();

            lastSentBigUpdateIndex.resize(currentBigTileIndex+1);

            for(int i=lastSize; i<=currentBigTileIndex; i++)
            {
                lastSentBigUpdateIndex[i] = -1;
            }
        }
    }
    while( currentBigTileIndex != lowDefRequiredStartIndex &&
           updateNumber == lastSentBigUpdateIndex[currentBigTileIndex] );

    if( currentBigTileIndex == lowDefRequiredStartIndex )
    {
        if( newClientJoined ) newClientJoined = false;
        else                  lowDefRequired  = false;
    }
    if( updateNumber == lastSentBigUpdateIndex[currentBigTileIndex] && ! newClientJoined ) return false;

    if( sendBigTileMessageFromUpdate(message,updateNumber,currentBigTileIndex, xBig, yBig) )
    {
        // Let's not send this again (unless it changes)
        if( ! newClientJoined )
        {
            // Not updating here allows us to send the screen twice on connect
            lastSentBigUpdateIndex[currentBigTileIndex] = updateNumber;
        }
        lastSendBigMessageNumber.append(updateNumber);
        lastSendBigMessageScreenIndex.append(currentBigTileIndex);

        if( lastSendBigMessageScreenIndex.length() > 100 )
        {
            lastSendBigMessageNumber.removeFirst();
            lastSendBigMessageScreenIndex.removeFirst();
        }

        // Paranoia
        repeatList.clear();

        return true;
    }

    return false;
}

bool viewBroadcaster::sendBigTileMessageFromUpdate(QByteArray &message, int &updateNumber, int &tileIndex, int &xBig, int &yBig)
{
    // //////////////////////////////////////////////////////
    // Generate a big tile message (NB this does not repeat.)
    message.resize(12);

    QByteArray jpgImage = *( screenGrabberTask->grabBigTileData(tileIndex,updateNumber) );
    int        imageSize = jpgImage.size();

    if( imageSize<119 )
    {
        qDebug() << "Big tile" << currentBigTileIndex << ": Smaller than the smallest valid jpeg. Sz =" << imageSize;
        return false;
    }

    uchar *msgData = (uchar *)message.data();

    msgData[0] = (updateNumber      ) & 255;
    msgData[1] = (updateNumber >>  8) & 255;
    msgData[2] = (updateNumber >> 16) & 255;
    msgData[3] = (updateNumber >> 24) & 255;

    lastUpdateNumber = updateNumber;

    msgData[4] = ACTION_BG_JPEG_TILE;
    msgData[5] = ((imageSize >> 16 ) & 0x0F) | ((screenGrabberTask->numBigPerSmallPower)<<4);

    msgData[6] = ( imageSize >>  8 ) & 0xFF;
    msgData[7] = ( imageSize       ) & 0xFF;

    msgData[8]  = (xBig >> 8) & 255;
    msgData[9]  =  xBig       & 255;
    msgData[10] = (yBig >> 8) & 255;
    msgData[11] =  yBig       & 255;

    // Append the actual image data
    message.append( jpgImage );

    return true;
}


void viewBroadcaster::buildAndSendViewMessage(int &updateNum)
{
    QByteArray message;

    if( repeatList.size() > 0 )
    {
        if( generateRepeatsMessage(message,updateNum) < 1 )
        {
            // No repeat entries
            return;
        }

        //qDebug() << "small repeats";
    }
    else // Not a repeat message
    {
        if( ! sendingFullscreenUpdate && lastFullScreenSendTimer->elapsed() < 2000 )
        {
#if 0 // We send lo def updates regularly for the while screen: just do high def ones here
            // Updates (high or low def)
            if( lowDefRequired )
            {
                if( ! generateBigTileMessages(message,updateNum) )
                {
                    return;
                }

                qDebug() << "big change:" << currentBigTileIndex;
            }
            else
            {
                if( numOutOfDateBigTiles() > 0 ) // ( lastSentBigUpdateIndex.size()/8 ) )
                {
                    lowDefRequiredStartIndex = currentBigTileIndex;
                    lowDefRequired           = true;
                }
#endif
                if( ! generateSmallTileMessages(message,updateNum) )
                {
                    qDebug() << "small tile";
                    return;
                }
#if 0
                qDebug() << "small update:";
            }
#endif
        }
        else
        {
            // Send full screen in low def (whether or not it has changed)
            if( ! sendingFullscreenUpdate )
            {
                qDebug() << "Start lo def full send...";

                lastFullScreenSendTimer->restart();
                fullscreenUpdateTileNum = 0;
                sendingFullscreenUpdate = true;
            }

            int  xBig, yBig;

            screenGrabberTask->nextBigTileIndexAndCoords(fullscreenUpdateTileNum, updateNum, xBig, yBig);

            // Allow for changing array sizes
            if( fullscreenUpdateTileNum >= lastSentBigUpdateIndex.size() )
            {
                int lastSize = lastSentBigUpdateIndex.size();

                lastSentBigUpdateIndex.resize(fullscreenUpdateTileNum+1);

                for(int i=lastSize; i<=fullscreenUpdateTileNum; i++)
                {
                    lastSentBigUpdateIndex[i] = -1;
                }
            }

            // Stop on error, or back to the start
            if( ! sendBigTileMessageFromUpdate(message,updateNum, fullscreenUpdateTileNum,xBig,yBig) )
            {
                // Stop refresh & don't send this time
                sendingFullscreenUpdate = false;

                // All small tiles should be resent
                lastSentSmallUpdateIndex.fill(-1);

                return;
            }

            if( fullscreenUpdateTileNum == 0 )
            {
                // Stop refresh & don't send this time
                sendingFullscreenUpdate = false;
            }

//            qDebug() << "Full screen resend" << fullscreenUpdateTileNum << updateNum;
        }
    }

    lastUpdateNumber = updateNum;

    // We've successfully generated a message. Send it...
    sendSocket->writeDatagram(message, QHostAddress::Broadcast, BROADCAST_VIEW_PORT);
    sendSocket->waitForBytesWritten();

    sentBytes += message.size(); // qDebug() << "sentBytes =" << sentBytes;
}


void viewBroadcaster::pauseSending(void)
{
    qDebug() << "pause viewBroadcaster";

    pause->lock();
}

void viewBroadcaster::resumeSending(void)
{
    qDebug() << "re-start viewBroadcaster";

    pause->unlock();
}



void viewBroadcaster::run()
{
    char               buf[BROADCAST_VIEW_PROBE_MESSAGE_SZ]; // 1024
    udp_view_packet_t *rxPacket = (udp_view_packet_t *)buf;
    udp_view_packet_t  txPacket;
    QHostAddress       sendAddress;
    quint16            sendPort;
    int                msDelayTillNextSend = 20;
    int                lastSendDelay       = 0;
    int                w, h;
    int                updateNum = -1;

    qDebug() << "Start broadcast server main loop.";

    sendSocket = new QUdpSocket(); // Want to create this in the child thread
    sendSocket->bind(BROADCAST_VIEW_PROBE_PORT);

    screenGrabberTask->addAUser();
    screenGrabberTask->waitForReady();

    qDebug() << "Veiw broadcast can continue as screen grabber is ready...";

    keepBroadcastServerAvailable = true;
    sentBytes                    = 0;

    QTime lastSendTime;
    lastSendTime.start();
    lastFullScreenSendTimer = new QTime;
    lastFullScreenSendTimer->start();

    // Main loop: forever, wait until we have new data then send it to the client.
    while( keepBroadcastServerAvailable )
    {
        if( sendSocket->hasPendingDatagrams() )
        {
            // Deal with any JOIN messages (respond) or ACKs (update data)
            if( sendSocket->readDatagram(buf, BROADCAST_VIEW_PROBE_MESSAGE_SZ,
                                        &sendAddress, &sendPort) > 0 )
            {
                qDebug() << "Rx data";
                switch( rxPacket->body.msg_type )
                {
                case ACTION_PROBE_VIEW_BROADCAST:

                    qDebug() << "Received view broadcast probe message. Resp Sz =" << sizeof(txPacket);

                    txPacket.update_num    = 0;
                    txPacket.body.msg_type = ACTION_VIEW_BROADCAST_ACK;

                    sendSocket->writeDatagram((char *)&txPacket,sizeof(txPacket),sendAddress,BROADCAST_VIEW_PORT);
                    sendSocket->waitForBytesWritten();

                    sentBytes += sizeof(txPacket);

                    // Send our display dimensions

                    txPacket.update_num    = 0;
                    txPacket.body.msg_type = ACTION_SCREENSIZE;

                    screenGrabberTask->updateScreenDimensions(w,h);

                    txPacket.body.data[4] = (w >> 8);
                    txPacket.body.data[5] =  w;
                    txPacket.body.data[6] = (h >> 8);
                    txPacket.body.data[7] =  h;

                    sendSocket->writeDatagram((char *)&txPacket,sizeof(udp_view_packet_t),sendAddress,BROADCAST_VIEW_PORT);
                    sendSocket->waitForBytesWritten();

                    sentBytes += sizeof(txPacket);

                    if( ! lowDefRequired )
                    {
                        lowDefRequiredStartIndex = currentBigTileIndex;
                        lastSentBigUpdateIndex.fill(-1);
                        lowDefRequired           = true;
                        newClientJoined          = true;
                    }
                    break;

                case ACTION_VIEW_BROADCAST_CLIENT_STATUS:

                    // Check for "lost" packets
                    qDebug() << "Lost packets: #" << rxPacket->body.view_ack.num_lost
                             << "slots"
                                << rxPacket->body.view_ack.lost_index[0]
                                << rxPacket->body.view_ack.lost_index[1]
                                << rxPacket->body.view_ack.lost_index[2]
                                << rxPacket->body.view_ack.lost_index[3]
                                << rxPacket->body.view_ack.lost_index[4];
                    for(int lost=0; lost<MAX_TRACKED_LOST_PACKETS; lost++)
                    {
                        if( rxPacket->body.view_ack.lost_index[lost] != INVALID_MESSAGE_INDEX )
                        {
                            int bigIndex = findBigIndexOfLostMessage(rxPacket->body.view_ack.lost_index[lost]);
                            if( bigIndex>=0 )
                            {
                                lastSentBigUpdateIndex[bigIndex] = -1;
                            }
                        }
                    }
                    break;

                default:

                    qDebug() << "Bad message on broadcast socket:"
                             << rxPacket->body.msg_type
                             << "(" << ACTION_DEBUG(rxPacket->body.msg_type) << ")";
                    break;
                }
            } else qDebug() << "Failed to read pending datagram.";
        }
        else
        {
            // Delay (calculate this when working)
            if( msDelayTillNextSend != lastSendDelay )
            {
                qDebug() << "Delay(ms) =" << msDelayTillNextSend;
                lastSendDelay = msDelayTillNextSend;
            }
            QThread::msleep(msDelayTillNextSend);

            // Send a "connected pens" message at most once every two seconds
            if( lastSendTime.elapsed() > 2000 )
            {
                // Monitor tx rate
                currentTxRateKbs = (sentBytes*8*1000.0 / lastSendTime.elapsed())/1024.0; // * 1000 as elapsed is in ms
                qDebug() << "Tx rate =" << currentTxRateKbs << "K Bits/sec";

                // Adjust the delay if out by 15% or more
                double diff = dataRateBitPerSec - currentTxRateKbs;
                if( fabs(diff/dataRateBitPerSec) > 0.15 )
                {
                    if( diff<0.0 )                     { msDelayTillNextSend ++; qDebug() << "Adjust Tx rate up to"   << msDelayTillNextSend << "diff =" << diff; }
                    else if( msDelayTillNextSend > 3 ) { msDelayTillNextSend --; qDebug() << "Adjust Tx rate down to" << msDelayTillNextSend << "diff =" << diff; }
                    //qDebug() << "Adjust Tx rate to" << msDelayTillNextSend;
                }

                sentBytes = 0;

                lastSendTime.restart();

                qDebug() << "Send connected pens message.";

                txPacket.update_num    = updateNum;
                txPacket.body.msg_type = ACTION_PENS_PRESENT; // Importantly, also sets othe bytes to zero.

                for(int p=0; p<MAX_SCREEN_PENS; p++)
                {
                    if( penIsActive[p] )
                    {
                        int byte = 3 - (p>>8);
                        txPacket.body.data[byte] |= (1<<(p&7));
                    }
                }

                sendSocket->writeDatagram((char *)&txPacket,sizeof(udp_view_packet_t),sendAddress,BROADCAST_VIEW_PORT);
                sendSocket->waitForBytesWritten();

                sentBytes += sizeof(udp_view_packet_t); // qDebug() << "sentBytes =" << sentBytes;
            }
            else
            {
                buildAndSendViewMessage(updateNum);
            }
        }

        // Allow for pause/unpause
#if 0
        // This stops the broadcast server from starting.
        pause->lock();   // wait
        pause->unlock(); // we don't actually want to hold this
#endif
    }
}

double viewBroadcaster::getCurrentSendRateMbs(void)
{
    return currentTxRateKbs/1024.0;
}

int viewBroadcaster::findBigIndexOfLostMessage(int messageNum)
{
    for(int chk=lastSendBigMessageNumber.length()-1; chk>=0; chk--)
    {
        if( messageNum == lastSendBigMessageNumber[chk] ) return lastSendBigMessageScreenIndex[chk];
    }
    return -1;
}
