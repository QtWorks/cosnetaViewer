#include "radarResponder.h"


radarResponder::radarResponder(savedSettings *settingRef, QObject *parent) : QObject(parent)
{
    settings = settingRef;
}


radarResponder::~radarResponder()
{
    delete socket;
}


void radarResponder::getMulticastConnection(void)
{
    socket   = new QUdpSocket;

    if( ! socket->bind(QHostAddress::Any, 14755, QUdpSocket::ShareAddress) )
    {
        qDebug() << "Failed to BIND RADAR socket.";
    }

    qDebug() << "Use iPV4 broadcast for now...";
#if 0
    if( socket->joinMulticastGroup(QHostAddress("224.0.172.190")) )
    {
        qDebug() << "Failed to join multicast group.";
    }
#endif
}


void radarResponder::doConnect(QThread &cThread)
{
    connect(&cThread, SIGNAL(started()), this, SLOT(answerThread()) );
}


void radarResponder::answerThread()
{
    int            delay = 500;  // Delay before coming out of readWait. Default: 0.5s
    QByteArray     dataPacket;
    net_message_t *msgIn;
    net_message_t  response;
    QHostAddress   senderAddr;
    quint16        senderPort;
    int            sz;

    quitResponder = false;

    // Get a multicast connection
    getMulticastConnection();

    // Keep on looking for packets and responding to them.
    // Need a random delay so everyone doesn't respond at once.

    while( ! quitResponder )
    {
        if( socket->waitForReadyRead(delay) )
        {
            // Read the message.

            // Ensure that the receive buffer is the right size for the new packet.
//            dataPacket.resize(receiveSocket.pendingDatagramSize());
            sz = socket->pendingDatagramSize();
            dataPacket.fill(0,sz);

            // Data pending: read it.
            socket->readDatagram(dataPacket.data(), sz,
                                       &senderAddr, &senderPort);
            msgIn = (net_message_t *)dataPacket.data();

            if( msgIn->messageType == NET_RADAR_PULSE     &&
                dataPacket.size()  == sizeof(net_message_t)  )
            {
                response.messageType = NET_RADAR_ECHO;

                QByteArray name = settings->getSessionName().toUtf8();

                int len = name.size()<PEN_NAME_SZ ? name.size() : PEN_NAME_SZ-1;
                memcpy(response.msg.radar.session_name, name.data(), len);
                response.msg.radar.session_name[len] = (char)0;

                int  numSlots = 1;
                response.msg.radar.allowed_new_connections[0] = numSlots / 256;
                response.msg.radar.allowed_new_connections[1] = numSlots & 255;

                delay           = qrand() % 500;
                if( delay < 0 ) delay = - delay;

                {
                    static int dbgSeqNum = 0;
                    qDebug() << dbgSeqNum << "Rx RADAR: Echo echo echo" << sz
                             << "to" << senderAddr.toString()
                             << "rx sender port =" << senderPort;
                    dbgSeqNum ++;
                }

                // Timeout: If transmit was pending, send it

                socket->writeDatagram((char *)&response,sizeof(response),senderAddr,14759);
                delay = 500;
            } else qDebug() << "Discard" << dataPacket.size() << "bytes for message type"
                            << ((int)(msgIn->messageType))
                            << "Client" << senderAddr.toString();
        }
    }
#if 0
    if( socket->leaveMulticastGroup( QHostAddress("224.0.172.190") ) )
    {
        qDebug() << "Failed to leave multicast group.";
    }
#endif
    qDebug() << "** exiting multicast radar responder.";
}


void radarResponder::stopCommand(void)
{
    quitResponder = true;
}
