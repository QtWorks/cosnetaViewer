#include "networkReceiver.h"

#include <QThread>
#include <QString>
#include <QtNetwork>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QList>

#include "net_connections.h"

#define UDP_TIMEOUT_MS 4000

#ifndef Q_OS_WIN
#include <unistd.h>
#endif

#include <iostream>

// Note that this refers to a different thread: beware.
#include "GUI.h"
extern CosnetaGui *gui;


QString    networkDebugString = "Unset";   // Used in debug displays.
static int rateReduce = 0;

networkReceiver::networkReceiver(sysTray_devices_state_t *table, QObject *parent) : QObject(parent)
{
    sharedTable = table;

    netIface = new QNetworkInterface;

    haveSessionKey = false;

    screenWidth  = 1980;
    screenHeight = 1080;

    state     = "Not running.";
    ipAddress = "-No IP address-";
#ifdef USE_UDP
    portNum   = NET_PEN_CTRL_PORT;
#else
    portNum   = -1;
#endif
}

void networkReceiver::updateScreenSize(int w, int h)
{
    screenWidth  = w;
    screenHeight = h;
}

networkReceiver::~networkReceiver()
{
}

void networkReceiver::doConnect(QThread &cThread)
{
    connect(&cThread, SIGNAL(started()), this, SLOT(readThread()) );
}

#ifndef USE_UDP

// ///
// TCP
// ///

void networkReceiver::readThread()
{
    bool keepGoing       = true;
    bool timedOut        = false;

    ipAddress = "0.0.0.0";

    while( keepGoing )
    {
        // Wait for an external IP address

        bool foundExternalIP = false;

        while( ! foundExternalIP && keepGoing )
        {
            state = "Waiting for external port.";

            QList<QHostAddress> ipAddresses = QNetworkInterface::allAddresses();

            if( ! ipAddresses.empty() )
            {
                for(int n=0; n<ipAddresses.size(); n++)
                {
                    if( ipAddresses.at(n) != QHostAddress::LocalHost &&
                        ipAddresses.at(n).toIPv4Address()               )
                    {
                        ipAddress       = ipAddresses.at(n).toString();
                        foundExternalIP = true;
//                    break;
                        std::cerr << "Found IP: " << ipAddress.toStdString() << std::endl;
                    }
                }
            }

            // If we don't have an IP, we want to periodically check - 2 sec pause
            if( ! foundExternalIP  )
            {
#ifdef Q_OS_WIN
                Sleep(2000);
#else
                sleep(2);
#endif
            }
        }

        if( foundExternalIP )
        {
            // Now, let's be a server

            QTcpServer server;

            state = "Awaiting connection.";

            server.listen(QHostAddress(ipAddress));
            portNum = server.serverPort();

            timedOut = true;

            while( timedOut && keepGoing )
            {
                server.waitForNewConnection(2000,&timedOut);
            }

            if( ! timedOut )
            {
                QTcpSocket *newConnection = server.nextPendingConnection();

                newConnection->setSocketOption(QAbstractSocket::LowDelayOption,1);

                updateFromSocketConnection(newConnection);
            }
        }
    }
}


void networkReceiver::updateFromSocketConnection(QTcpSocket *cnx)
{
    char          startChar;
    bool          foundSlot = false;
    int           index;
    unsigned char rdBuffer[16];
    int           x,y, buttons;

    // Find a free slot
    sharedTable->addRemoveDeviceMutex->lock();

    for(int srch=0; srch<MAX_PENS; srch++)
    {
        if( ! COS_PEN_IS_ACTIVE(sharedTable->pen[srch]) )
        {
            COS_PEN_SET_ALL_FLAGS(sharedTable->pen[srch],COS_PEN_ACTIVE|COS_PEN_ACTIVE);
//            sharedTable->locCalc[srch]->updateScreenGeometry(&desktop);
            foundSlot = true;
            index     = srch;
            break;
        }
    }

    sharedTable->addRemoveDeviceMutex->unlock();

    if( ! foundSlot )
    {
        balloonMessage(tr("Rejected incomming conection as no free slots."));
        cnx->close();
        return;
    }

    balloonMessage("New network controller connection.");

    // Keep reading until connection fails

    rdBuffer[0] = 'N';
    while( cnx->state() == QAbstractSocket::ConnectedState &&
           rdBuffer[0]  != '#'      &&      rdBuffer[1] !='#' )
    {
        rdBuffer[1] = rdBuffer[0];
        cnx->read(((char *)rdBuffer),1);
    }

    while( cnx->state() == QAbstractSocket::ConnectedState )
    {
        // Using this VVVV knackers the data contents - it must consume some data :O
        //if( ! cnx->waitForReadyRead(1000) ) continue;
        if( cnx->read(((char *)rdBuffer),2+2+2+2) <= 0 )  // X,Y,buttons,'##'
            continue;

        x = (screenWidth *(256*rdBuffer[0] + rdBuffer[1]))/10000;
        y = (screenHeight*(256*rdBuffer[2] + rdBuffer[3]))/10000;
        buttons = 256*rdBuffer[4] + rdBuffer[5];

        sharedTable->pen[index].screenX = x;
        sharedTable->pen[index].screenY = y;
        sharedTable->pen[index].buttons = buttons;
#if 1
        {
            if( rateReduce++ > 100 )
            {
                rateReduce = 0;
                networkDebugString = QString("netPos (%1, %2): 0x%3").arg(x).arg(y).arg(buttons,0,16);
            }
        }
#endif
    }

    // And clean up
#ifdef Q_OS_WIN
    Sleep(1000);
#else
    sleep(1);
#endif
    COS_PEN_SET_ALL_FLAGS(sharedTable->pen[index],COS_PEN_INACTIVE);
    cnx->close();
}
#else

// ///
// UDP
// ///

QString networkReceiver::listIPAddresses(void)
{
    ipAddress = "";

    // Look for an IP address

//    bool foundExternalIP = false;

    state = "Waiting for external port.";

    QList<QHostAddress> ipAddresses = QNetworkInterface::allAddresses();

    if( ! ipAddresses.empty() )
    {
        for(int n=0; n<ipAddresses.size(); n++)
        {
            if( ipAddresses.at(n) != QHostAddress::LocalHost &&
                ipAddresses.at(n).toIPv4Address()               )
            {
                ipAddress      += QString("[") + ipAddresses.at(n).toString() + QString("] ");
                std::cerr << "Found IP: " << ipAddress.toStdString() << std::endl;
            }
        }
    }

    return ipAddress;
}

void networkReceiver::readThread()
{
    bool           keepGoing       = true;
    QUdpSocket     receiveSocket(this);
    QHostAddress   senderAddr;
    quint16        senderPort;
    QHostAddress   activeSenderAddr[MAX_IP_CNX];   // Arbritrary maximum number of IP connections to allow
    quint16        activeSenderPort[MAX_IP_CNX];
    int            lastReceive[MAX_IP_CNX];
    uint32_t       lastRxActionSequenceNum[MAX_IP_CNX];
    QTime          timer;
    int            s;
    quint32        action;
    int            keypress;
    bool           sendShift;
    bool           sendControl;
    bool           inUse;
    QByteArray     dataPacket;
    net_message_t *msgIn;
    int            packetTarget;
    int            x, y, buttons;

    timer.start();

    maxNumActiveSenders = MAX_IP_CNX;
    numSendersActive    = 0;

    for(s=0; s<MAX_IP_CNX; s++)
    {
        senderActive[s]            = false;
        lastRxActionSequenceNum[s] = 0;
    }

    receiveSocket.bind(QHostAddress::Any, portNum, QAbstractSocket::ShareAddress);

    while( keepGoing )
    {
        // Wait for up to 4 seconds (if no data, close all connections)
        if( receiveSocket.waitForReadyRead(UDP_TIMEOUT_MS) )
        {
            // Ensure that the receive buffer is the right size for the new packet.
//            dataPacket.resize(receiveSocket.pendingDatagramSize());
            dataPacket.fill(0,receiveSocket.pendingDatagramSize());

            // Data pending: read it.
            if( receiveSocket.readDatagram(dataPacket.data(),
                                           receiveSocket.pendingDatagramSize(),
                                           &senderAddr,      &senderPort) >= sizeof(net_message_t) )
            {
                msgIn = (net_message_t *)dataPacket.data();

                if( haveSessionKey ) passwordDecryptMessage(msgIn);

                // Have data. Lookup sender, or is it a new sender?
                inUse = false;

                for(s=0; s<MAX_IP_CNX && ! inUse; s++)
                {
                    if( senderActive[s] && (activeSenderAddr[s] == senderAddr) &&
                                           (activeSenderPort[s] == senderPort)   )
                    {
                        packetTarget = s;
                        inUse        = true;
                    }
                }

                // Verifications

                if( msgIn->messageType != msgIn->messageTypeCheck )
                {
                    qDebug() << "Received a bad type check packet from"
                             << senderAddr << senderPort
                             << "type:" << (int)(msgIn->messageType)
                             << "check:" << (int)(msgIn->messageTypeCheck);

                    msgIn->messageType = NET_INVALID_PACKET_TYPE;
                }
                else if( msgIn->protocolVersion != PROTOCOL_VERSION )
                {
                    qDebug() << "Received a bad protocol version packet from" << senderAddr << senderPort;

                    msgIn->messageType = NET_INVALID_PACKET_TYPE;
                }


                // If was a JOIN request packet, then respond
                if( msgIn->messageType == NET_JOIN_REQ_PACKET        ||
                    msgIn->messageType == NET_JOIN_REQ_PACKET_NO_ECHO )
                {
                    net_message_t reply;
                    int           sharedTableIndex = MAX_PENS;

                    // If it's from a new sender, is there space for it?
                    if( ! inUse && numSendersActive < maxNumActiveSenders  )
                    {
                        // Try and find an available slot
                        // Find a free slot
                        sharedTable->addRemoveDeviceMutex->lock();

                        for(sharedTableIndex=0; sharedTableIndex<MAX_PENS; sharedTableIndex++)
                        {
                            if( ! COS_PEN_IS_ACTIVE(sharedTable->pen[sharedTableIndex]) )
                            {
                                // Look for an available slot in IP senderTable
                                for(s=0; s<MAX_IP_CNX; s++) if( ! senderActive[s] ) break;
                                if( s<=MAX_IP_CNX )
                                {
                                    senderActive[s]     = true;
                                    lastReceive[s]      = timer.elapsed();
                                    senderTarget[s]     = sharedTableIndex;
                                    activeSenderPort[s] = senderPort;
                                    activeSenderAddr[s] = senderAddr;
                                    numSendersActive ++;

                                    // Now store settings
                                    memcpy(sharedTable->settings[sharedTableIndex].users_name,msgIn->msg.join.pen_name,MAX_USER_NAME_SZ);
                                    sharedTable->settings[sharedTableIndex].users_name[MAX_USER_NAME_SZ-1] = (char)0;
                                    sharedTable->settings[sharedTableIndex].colour[0]   = msgIn->msg.join.colour[0];
                                    sharedTable->settings[sharedTableIndex].colour[1]   = msgIn->msg.join.colour[1];
                                    sharedTable->settings[sharedTableIndex].colour[2]   = msgIn->msg.join.colour[2];
                                    sharedTable->settings[sharedTableIndex].colour[3]   = 255; // Not used, but if becomes transparecy, set to full
                                    sharedTable->settings[sharedTableIndex].sensitivity = 100; // Not used, but visible in settings
                                    for(int b=0; b<NUM_MAPPABLE_BUTTONS; b++)     // Not used, but visible in settings
                                    {
                                        sharedTable->settings[sharedTableIndex].button_order[b] = b;
                                    }

                                    balloonMessage( QString("New device: %1")
                                                    .arg(sharedTable->settings[sharedTableIndex].users_name) );

                                    qDebug() << "New network pen"
                                             << QString(sharedTable->settings[sharedTableIndex].users_name)
                                             << "colour" << sharedTable->settings[sharedTableIndex].colour[0]
                                                         << sharedTable->settings[sharedTableIndex].colour[1]
                                                         << sharedTable->settings[sharedTableIndex].colour[2];

                                    // Let's start with vaguely sensible pen data and no buttons pressed
                                    sharedTable->pen[sharedTableIndex].screenX = 0;
                                    sharedTable->pen[sharedTableIndex].screenY = 0;
                                    sharedTable->pen[sharedTableIndex].buttons = 0;

                                    QByteArray addrStr = senderAddr.toString().toLatin1();
                                    memcpy( &(sharedTable->networkPenAddress[sharedTableIndex][0]),
                                            addrStr.constData(), addrStr.length() );

                                    // Grab slot
                                    if( msgIn->messageType == NET_JOIN_REQ_PACKET )
                                    {
                                        COS_PEN_SET_ALL_FLAGS(sharedTable->pen[sharedTableIndex],
                                                              COS_PEN_ACTIVE|COS_PEN_FROM_NETWORK|COS_PEN_ALLOW_REMOTE_VIEW);
                                    }
                                    else if( msgIn->messageType == NET_JOIN_REQ_PACKET_NO_ECHO )
                                    {
                                        COS_PEN_SET_ALL_FLAGS(sharedTable->pen[sharedTableIndex],
                                                              COS_PEN_ACTIVE|COS_PEN_FROM_NETWORK );
                                    }

                                    // Tell sppComms to copy out the settings
                                    sharedTable->settings_changed_by_systray[sharedTableIndex] = COS_TRUE;

                                    // Start network connections as overlay-only (unlike freeRunners)
                                    sharedTable->gestCalc[sharedTableIndex].forceCurrentControllerMode(NORMAL_OVERLAY);

                                    // And kick the GUI so it can update the displayed pen settings
                                    gui->penChanged(sharedTableIndex);
                                }

                                break;
                            }
                        }

                        // Done adding to the shared table
                        sharedTable->addRemoveDeviceMutex->unlock();
                    }

                    // The inUse check is to allow resends of ACCEPT as UDP is by definition, unreliable.
                    if( sharedTableIndex >= MAX_PENS && ! inUse )
                    {
                        qDebug() << "Pen join rejected from" << senderAddr << senderPort;

                        reply.messageType = NET_JOIN_REJECT;
                    }
                    else
                    {
                        qDebug() << "Pen join accepted from" << senderAddr << senderPort;

                        reply.messageType      = NET_JOIN_ACCEPT;
                        reply.msg.join.pen_num = sharedTableIndex;
                    }

                    if( haveSessionKey ) passwordEncryptMessage(&reply);

                    // Reply to client, so it knows to start sending data
    //                qDebug() << "Respond to a join packet from" << senderAddr << senderPort << "with type" << (0+reply.messageType)
    //                         << "message" << QByteArray((char *)&reply,sizeof(reply)).toHex();

                    receiveSocket.writeDatagram((char *)&reply,sizeof(reply), senderAddr,  senderPort );
                }
                else if( msgIn->messageType == NET_PEN_LEAVE_PACKET )
                {
                    // Look up the corresponding entry
                    for(int sender=0; sender<MAX_PENS; sender++)
                    {
                        // NB Don't test for senderActive[] as client may not have received the LEAVE_ACK

                        if( (activeSenderAddr[sender] == senderAddr) &&
                            (activeSenderPort[sender] == senderPort)    )
                        {
                            senderActive[sender] = false;

                            inUse = false;

                            numSendersActive --;

                            net_message_t reply;

                            reply.messageType      = NET_PEN_LEAVE_ACK;
                            reply.msg.join.pen_num = sender;

                            if( haveSessionKey ) passwordEncryptMessage(&reply);

                            receiveSocket.writeDatagram((char *)&reply,sizeof(reply), senderAddr,  senderPort );

                            qDebug() << "ACKed PEN LEAVE request.";

                            COS_PEN_SET_ALL_FLAGS(sharedTable->pen[senderTarget[sender]], COS_PEN_INACTIVE );

                            break;
                        }
                    }
                }
            } else { qDebug() << "Received an input message that is too small"; }

            // If we were able, we will have a pen slot now

            if( inUse )
            {
                msgIn = (net_message_t *)dataPacket.data();

                if( msgIn->messageType == NET_DATA_PACKET )
                {
                    // Update with the received data
                    x       = (screenWidth*(256*msgIn->msg.data.x[0] + msgIn->msg.data.x[1]))/10000;
                    y       = (screenHeight*(256*msgIn->msg.data.y[0] + msgIn->msg.data.y[1]))/10000;
                    buttons = msgIn->msg.data.buttons;

                    int  index = senderTarget[packetTarget];

                    if( x>=0 && x<screenWidth && y>=0 && y<screenHeight )
                    {
                        sharedTable->pen[index].screenX = x;
                        sharedTable->pen[index].screenY = y;

                        // TODO: manage lastButtons and buttonChangeNumber to mitigate against UDP packet loss
                        sharedTable->pen[index].buttons = buttons;

                        sharedTable->gestCalc[index].update( &(sharedTable->pen[index]) );

                        pen_modes_t oldMode = sharedTable->penMode[index];
                        sharedTable->penMode[index] = sharedTable->gestCalc[index].mouseModeSelected();
                        if( oldMode != sharedTable->penMode[index] )
                        {
                            gui->penChanged(index);
                        }

                        if( sharedTable->gestCalc[index].gestureGetKeypress(&keypress, &sendShift, &sendControl) )
                        {
                            // File this away in the table

                            sharedTable->gesture_sequence_num[index] ++;
                            sharedTable->gesture_key[index]      = keypress;
                            sharedTable->gesture_key_mods[index] =
                                                    (sendShift   ? KEY_MOD_SHIFT : 0 ) |
                                                    (sendControl ? KEY_MOD_CTRL  : 0 )  ;
                        }
                        if( sharedTable->gestCalc[index].getPresentationAction(&action) )
                        {
                            // File this away in the table

                            sharedTable->pen[index].app_ctrl_action = action;
                            sharedTable->pen[index].app_ctrl_sequence_num ++;
                        }

                    }
                    else
                    {
                        qDebug() << "Bad packet on Rcv slot " << packetTarget <<
                                    "Coords:" << x << y << "Unscaled coords" <<
                                    (256*msgIn->msg.data.x[0] + msgIn->msg.data.x[1]) <<
                                    (256*msgIn->msg.data.y[0] + msgIn->msg.data.y[1]) <<
                                    "Message:" << dataPacket.toHex();
                    }

                    // Only look for actions (including characters) in big enough messages
                    // as some senders wont include them.
                    if( dataPacket.size() >= sizeof(data_packet_t) )
                    {
                        int app_ctrl_action_in = (msgIn->msg.data.app_ctrl_action[0]<<24) +
                                                 (msgIn->msg.data.app_ctrl_action[1]<<16) +
                                                 (msgIn->msg.data.app_ctrl_action[2]<< 8) +
                                                 (msgIn->msg.data.app_ctrl_action[3]    );
                        int app_ctrl_sequence_in = (msgIn->msg.data.app_ctrl_sequence_num[0]<<24) +
                                                   (msgIn->msg.data.app_ctrl_sequence_num[1]<<16) +
                                                   (msgIn->msg.data.app_ctrl_sequence_num[2]<< 8) +
                                                   (msgIn->msg.data.app_ctrl_sequence_num[3]    );
                        if( app_ctrl_sequence_in != lastRxActionSequenceNum[index] )
                        {
                            lastRxActionSequenceNum[index]          = app_ctrl_sequence_in;
                            sharedTable->pen[index].app_ctrl_action = app_ctrl_action_in;
                            sharedTable->pen[index].app_ctrl_sequence_num ++;

                            // And check for a mode set command
                            if( APP_CTRL_IS_PEN_MODE(app_ctrl_action_in) )
                            {
                                switch(APP_CTRL_PEN_MODE_FROM_RX(app_ctrl_action_in) )
                                {
                                case APP_CTRL_PEN_MODE_OVERLAY:

                                    qDebug() << "Overlay mode received...";
                                    sharedTable->gestCalc[index].forceCurrentControllerMode(NORMAL_OVERLAY);
                                    sharedTable->penMode[index] = sharedTable->gestCalc[index].mouseModeSelected();
                                    gui->penChanged(index);
                                    break;

                                case APP_CTRL_PEN_MODE_PRESNTATION:

                                    qDebug() << "Presentation mode received...";
                                    sharedTable->gestCalc[index].forceCurrentControllerMode(PRESENTATION_CONTROLLER);
                                    sharedTable->penMode[index] = sharedTable->gestCalc[index].mouseModeSelected();
                                    gui->penChanged(index);
                                    break;

                                case APP_CTRL_PEN_MODE_MOUSE:

                                    qDebug() << "Mouse mode received...";
                                    sharedTable->gestCalc[index].forceCurrentControllerMode(DRIVE_MOUSE);
                                    sharedTable->penMode[index] = sharedTable->gestCalc[index].mouseModeSelected();
                                    gui->penChanged(index);
                                    break;
                                }
                            }
                        }
                    }
                }
                else if( msgIn->messageType != NET_JOIN_REQ_PACKET )
                {
                    qDebug() << "Bad packet on Rcv slot " << dataPacket.toHex();
                }

                lastReceive[packetTarget] = timer.elapsed();
            } // inUse
        }

        // Look for any timed out connections
        int now = timer.elapsed();
        for(s=0; s<MAX_IP_CNX; s++)
        {
            if( senderActive[s] && (now-lastReceive[s]) > UDP_TIMEOUT_MS )
            {
                balloonMessage(QString("Removed device: %1")
                               .arg(sharedTable->settings[s].users_name));

                qDebug() << "Timeout network pen" << s << "as delay is" << (now-lastReceive[s]);

                COS_PEN_SET_ALL_FLAGS(sharedTable->pen[senderTarget[s]],COS_PEN_INACTIVE);
                senderActive[s]  = false;
                numSendersActive --;

                gui->penChanged(senderTarget[s]);
            }
        }
    }
}

#endif

bool networkReceiver::setMaxNumIPConnections(int maxConnections)
{
    if( maxConnections < 0 || maxConnections > MAX_IP_CNX )
    {
        qDebug() << "setMaxNumIPConnections: FAILED: bad max IP connextions:" << maxConnections;
        return false;
    }

    // TODO: do we fail if more than the current number are in use, or kick some off?

    // Update the maximum
    maxNumActiveSenders = maxConnections;
    return true;
}

int  networkReceiver::getMaxNumIPConnections(void)
{
    return maxNumActiveSenders;
}


bool  networkReceiver::tableEntryIsNetworkConnection(int tableIndex)
{
    for(int i=0; i<MAX_IP_CNX; i++)
    {
        if( senderActive[i] && senderTarget[i] == tableIndex ) return true;
    }

    return false;
}

void networkReceiver::stopCommand(void)
{
}

void networkReceiver::startCommand(void)
{
}

void networkReceiver::waitForStop(void)
{
}

void networkReceiver::setNewPassword(QString newPassword)
{
    QString trimmedPassword = newPassword.trimmed();

    if( trimmedPassword.length() > 0 )
    {
        sessionKey.resize(KEY_LENGTH);
        sessionKey.fill(0);

        // Get a fixed length password. If password is longer than key length, wrap and xor onto
        // previous data. If shorter, repeat the password characters.
        QByteArray pwdChar = newPassword.toLocal8Bit();

        for(int i=0; i<newPassword.length() && i<KEY_LENGTH; i++)
        {
            sessionKey[i % KEY_LENGTH] = sessionKey.at(i % KEY_LENGTH) ^ pwdChar[i % KEY_LENGTH];
        }

        // More muddling.
        for(int i=1; i<KEY_LENGTH; i++)
            sessionKey[i] = sessionKey.at(i) + sessionKey.at(i-1);

        haveSessionKey = true;
    }
    else
    {
        haveSessionKey = false;
    }
}


// Temp: just encrypt by xoring with key

static int saltChange = 1;

void networkReceiver::passwordEncryptMessage(net_message_t *msg)
{
    if( haveSessionKey )
    {
        msg->salt = saltChange;
        saltChange ++;

        char *key = sessionKey.data();
        char *dat = (char *)msg;

        for(int i=0; i<sizeof(net_message_t); i++)
        {
            dat[i] ^= key[i % KEY_LENGTH];
        }
    }
}

void networkReceiver::passwordDecryptMessage(net_message_t *msg)
{
    if( haveSessionKey )
    {
        saltChange ++;

        char *key = sessionKey.data();
        char *dat = (char *)msg;

        for(int i=0; i<sizeof(net_message_t); i++)
        {
            dat[i] ^= key[i % KEY_LENGTH];
        }
    }
}

void networkReceiver::connectionStatus(QString &hostIP, int &port, QString &status)
{
    hostIP = listIPAddresses();
    port   = portNum;
    status = QString("Num connections: %1").arg(numSendersActive);
}

