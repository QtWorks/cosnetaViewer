#include <QGridLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QUdpSocket>
#include <QHostAddress>
#include <QDir>
#include <QSettings>
#include <QTimer>
#include <QDebug>
#include <QKeyEvent>

#include "debugLogger.h"

#include "cosnetaAPI.h"
#define LOCAL_MOUSE_PEN (MAX_PENS)

#include "../systrayqt/net_connections.h"

#include "remoteConnectDialogue.h"

#define DEFAULT_POINTER_PEN_NAME      "Set device name."
#define DEFAULT_POINTER_PASSWORD      ""
#define DEFAULT_HOST_IP               "cosneta.example.com"   /* as every download looses last defaults */
#define DEFAULT_HOST_CONTROL_PORT      14753                  /* sysTray port number   */
#define DEFAULT_HOST_VIEW_PORT         14754                  /* freeStyle port number */
#define DEFAULT_HOST_CONTROL_PORT_STR "14753"                 /* sysTray port number   */
#define DEFAULT_HOST_VIEW_PORT_STR    "14754"                 /* freeStyle port number */

#define RADAR_IP_ADDRESS              "224.0.172.190"
#define RADAR_PORT_NUM                14755

#define APPLICATION_SETTINGS_FILENAME "freeStyleQt.ini"
#define SETS_POINTER_PENNAME          "last_remote/pointerPenName"
#define SETS_POINTER_PASSWORD         "last_remote/sessionPassword"
#define SETS_HOSTNAME                 "last_remote/hostName"
#define SETS_HOSTPORT                 "last_remote/hostPort"
#define SETS_HAS_HOSTVIEW             "last_remote/differentViewer"
#define SETS_HOSTVIEWNAME             "last_remote/hostViewRepeaterName"
#define SETS_HOSTVIEWPORT             "last_remote/hostViewRepeaterPort"


remoteConnectDialogue::remoteConnectDialogue(QUdpSocket *drawSkt, QWidget *parent) : QWidget(parent, Qt::Dialog)
{
    // Save data for future use...
    remoteSocket     = drawSkt;

    // Initialisations
    remoteConnection  = false;
    remotePenNum      = -1;
    passwordIsNotNull = false;

    // Set up the ping data packet
    memset(&dataPacket,0,sizeof(dataPacket));
    dataPacket.messageType      = NET_RADAR_PULSE;
    dataPacket.messageTypeCheck = NET_RADAR_PULSE;

    qDebug() << "Create remote connect dialogue";

    // Dialogue
    remoteConnectDialogueStatus = new QLabel(tr("Select freeStyle session to join."));

    QHBoxLayout *penNameLayout = new QHBoxLayout;

    pointerPenName      = new QLineEdit( DEFAULT_POINTER_PEN_NAME );
    QLabel *pointerName = new QLabel(tr("Device Name"));
    penNameLayout->addWidget(pointerName);
    penNameLayout->addWidget(pointerPenName);

    QHBoxLayout *passwordLayout = new QHBoxLayout;

    passwordEntry        = new QLineEdit();
    QLabel *passwordName = new QLabel(tr("Password"));
    passwordLayout->addWidget(passwordName);
    passwordLayout->addWidget(passwordEntry);

    userSelectedManualConfig = false;

    foundServersCombo = new QComboBox;
    foundServersCombo->setEditable(false);
    foundServersCombo->addItem(tr("Manual configuration."));
    foundServersCombo->insertSeparator(0);
    foundServersCombo->setCurrentIndex(1);

    connect(foundServersCombo,SIGNAL(currentIndexChanged(int)), this, SLOT(foundServerSelected(int)));

    foundServersCombo->hide();

    manualDataButton = new QPushButton(tr("Manual Remote Data"));
    connect(manualDataButton, SIGNAL(clicked()), this, SLOT(manualConnectData()) );

    QGridLayout *gridBox = new QGridLayout;

    QLabel *senderLabel         = new QLabel(tr("Controller"));
    QLabel *viewerLabel         = new QLabel(tr("Viewer"));
    QLabel *addressLabel        = new QLabel(tr("Session address:"));
    QLabel *portLabel           = new QLabel(tr("Session port:"));
    hostEdit       = new QLineEdit( DEFAULT_HOST_IP );
    portEdit       = new QLineEdit( DEFAULT_HOST_CONTROL_PORT_STR );
    hostViewEdit   = new QLineEdit( DEFAULT_HOST_IP );
    viewPortEdit   = new QLineEdit( DEFAULT_HOST_VIEW_PORT_STR );

    // The following coordinates are [row,column]
    gridBox->addWidget(senderLabel, 0,1);
    gridBox->addWidget(viewerLabel, 0,2);

    gridBox->addWidget(addressLabel,1,0);
    gridBox->addWidget(hostEdit,    1,1);
    gridBox->addWidget(hostViewEdit,1,2);

    gridBox->addWidget(portLabel,   2,0);
    gridBox->addWidget(portEdit,    2,1);
    gridBox->addWidget(viewPortEdit,2,2);

    remoteCustomConnect = new QCheckBox(tr("custom"));
//    remoteCustomConnect = new QCheckBox(tr("custom view configuration."));
    connect(remoteCustomConnect, SIGNAL(clicked()), this, SLOT(remoteCustomEnableChanged()) );

    QLabel *manualInstructions = new QLabel(tr("Type the IP address and port of "
                                               "the remote session"));

    QVBoxLayout *manualConfigBox = new QVBoxLayout;
    manualConfigBox->addItem(gridBox);
//    manualConfigBox->addWidget(remoteCustomConnect);
    gridBox->addWidget(remoteCustomConnect, 0,0);
    manualConfigBox->addWidget(manualInstructions);

    manualConfigArea = new QWidget;
    manualConfigArea->setLayout(manualConfigBox);

    manualConfigArea->hide();

    searchingForSessions = new QLabel(tr("Searching for sessions..."));
    searchingForSessions->setStyleSheet("QLabel { color: red }");

    QHBoxLayout *actionBox = new QHBoxLayout();
    connectButton          = new QPushButton(tr("Connect"));
    connect(connectButton, SIGNAL(clicked()), this, SLOT(remoteConnect()) );
    connectButton->hide();
    actionBox->addWidget(connectButton);
#ifdef Q_OS_ANDROID
    QPushButton *helpButton    = new QPushButton(tr("Help"));
    connect(helpButton,    SIGNAL(clicked()), this, SLOT(helpSelected()) );
    actionBox->addWidget(helpButton);
    if( debugAvailableToSend() )
    {
        sendDebugButton  = new QPushButton(tr("Send Debug"));
        connect(sendDebugButton,    SIGNAL(clicked()), this, SLOT(sendDebugSelected()) );
        actionBox->addWidget(sendDebugButton);
    }
    QPushButton *quitButton    = new QPushButton(tr("Quit"));
    connect(quitButton,    SIGNAL(clicked()), this, SLOT(quitProgramSelected()) );
#else
    QPushButton *quitButton    = new QPushButton(tr("Cancel"));
    connect(quitButton,    SIGNAL(clicked()), this, SLOT(rejectRemoteConnect()) );
#endif
    actionBox->addWidget(quitButton);

    hostViewEdit->setEnabled(false);
    viewPortEdit->setEnabled(false);

    qDebug() << "remoteConnectDialogue: Top level layout.";
    QVBoxLayout *vbox;
    vbox = new QVBoxLayout;
    vbox->addWidget(remoteConnectDialogueStatus);
    vbox->addStretch();
    vbox->addItem(penNameLayout);
    vbox->addItem(passwordLayout);
    vbox->addStretch();
    vbox->addWidget(foundServersCombo);
    vbox->addWidget(manualDataButton);
    vbox->addWidget(manualConfigArea);
    vbox->addWidget(searchingForSessions);
    vbox->addStretch();
    vbox->addItem(actionBox);

    setLayout(vbox);
}


void remoteConnectDialogue::startRemoteConnection(bool penIsMouse, char *penName, QColor penColour, int newParentHeight, int newParentWidth)
{
    // Store data for potential use on click event...
    penIsLocalPointer = penIsMouse;
    colour            = penColour;
    parentWidth       = newParentWidth;
    parentHeight      = newParentHeight;
    if( penName ) strcpy(penCStringName, penName);

    // Update with values from settings

    QString settingsFName = QDir::homePath() + "/" +
                            QString(APPLICATION_SETTINGS_FILENAME);
    if( QFile::exists(settingsFName) )
    {
        QSettings settings(settingsFName,QSettings::IniFormat);

        pointerPenName->setText(settings.value(SETS_POINTER_PENNAME,DEFAULT_POINTER_PEN_NAME).toString());
        passwordEntry->setText(settings.value(SETS_POINTER_PASSWORD,DEFAULT_POINTER_PASSWORD).toString());
        hostEdit->setText(settings.value(SETS_HOSTNAME,DEFAULT_HOST_IP).toString());
        portEdit->setText(settings.value(SETS_HOSTPORT,DEFAULT_HOST_CONTROL_PORT_STR).toString());
        hostViewEdit->setText(settings.value(SETS_HOSTVIEWNAME,DEFAULT_HOST_IP).toString());
        viewPortEdit->setText(settings.value(SETS_HOSTVIEWPORT,DEFAULT_HOST_VIEW_PORT_STR).toString());
        remoteCustomConnect->setChecked(settings.value(SETS_HAS_HOSTVIEW,false).toBool());

        if( remoteCustomConnect->isChecked() )
        {
            hostViewEdit->setEnabled(true);
            viewPortEdit->setEnabled(true);
        }
        else
        {
            hostViewEdit->setEnabled(false);
            viewPortEdit->setEnabled(false);
        }
    }

    initialPenName = pointerPenName->text();

    // And display the dialogue
    show();
    raise();

#ifdef Q_OS_ANDROID
    // Manually center on parent as it isn't done automatically. Sigh.
    QPoint topLeft( parentWidth/2  - width()/2,
                    parentHeight/2 - height()/2);

    move(topLeft);

    repaint();
#endif

    // Start the RADAR
    radarSocket.bind(14759,QUdpSocket::ShareAddress);
    qDebug() << "remoteConnectDialogue: bind() radarSocket" << radarSocket.socketDescriptor() << "to" << 14755;
    connect(&radarSocket, SIGNAL(readyRead()), this, SLOT(serverResponded()));

    QTimer::singleShot(100, this, SLOT(pingForServers()));
}

void remoteConnectDialogue::reStartRemoteConnection(int newParentHeight, int newParentWidth)
{
    // NB The NULL means that the name wont be updated
    startRemoteConnection(penIsLocalPointer,NULL,colour,newParentHeight,newParentWidth);
}


#ifdef Q_OS_ANDROID
bool remoteConnectDialogue::event(QEvent *e)
{
    // Check for tablet "Back Key"
    if( e->type() == QEvent::KeyRelease )
    {
        QKeyEvent *kev = static_cast<QKeyEvent *>(e);

        if( kev->key() == Qt::Key_Back )
        {
            quitProgramSelected();
        }
    }

    return QWidget::event(e);
}
#endif



void remoteConnectDialogue::setStatusString(QString newStatus)
{
    remoteConnectDialogueStatus->setText(newStatus);
}


bool remoteConnectDialogue::havePassword(void)
{
    return passwordIsNotNull;
}


void remoteConnectDialogue::remoteConnect()
{
    // Either use a manually set server, or a discovered one:

    bool  sessionIsManual;

    if( foundServersCombo->currentIndex() >= foundServersList.length() )
    {
        sessionIsManual = true;
    }
    else
    {
        sessionIsManual = false;
    }

    if( penIsLocalPointer )
    {
        QByteArray str;

        if( initialPenName != pointerPenName->text() )
        {
            str = pointerPenName->text().toUtf8();
        }
        else
        {
            str = QString("Unnamed device").toUtf8();
        }
        int len = str.length();

        if( len>MAX_USER_NAME_SZ-1 ) len = MAX_USER_NAME_SZ-1;

        memset(penCStringName,0,MAX_USER_NAME_SZ+1);
        memcpy(penCStringName, str.data(), len);
    }

    if( sessionIsManual )
    {
        // Manual config: Extract the definition of the server we wish to connect to
        hostAddr    = QHostAddress(hostEdit->text());
        hostPortNum = portEdit->text().toInt();

        if( remoteCustomConnect->isChecked() )
        {
            // Read from the edit boxes
            hostViewAddr    = QHostAddress(hostViewEdit->text());
            hostViewPortNum = viewPortEdit->text().toInt();
        }
        else
        {
            // Use default: same host, port number+1
            hostViewAddr    = hostAddr;
            hostViewPortNum = hostPortNum + 1;
        }

        remoteConnectDialogueStatus->setText(tr("Attempting to connect to manually configured remote session."));
    }
    else
    {
        // Use discovered session
        hostAddr        = foundServersList[foundServersCombo->currentIndex()].IPAddress;
        hostPortNum     = 14753;
        hostViewAddr    = hostAddr;
        hostViewPortNum = hostPortNum + 1;
    }

    qDebug() << "Host controller:" << hostAddr << hostPortNum << "host view" << hostViewAddr << hostViewPortNum;
#if 1
    // Use this if only connect when want to edit/update the session
    emit remoteHostSelected();
#else
    // Attempt to connect to it

    // Try the host (sets remoteConnection & remotePenNum)

    remoteConnection = connectPenToHost();

    if( remoteConnection )
    {
#endif
        if( sessionIsManual || initialPenName != pointerPenName->text() )
        {
            // Store this connetion as last successful connected host
            QString   settingsFName = QDir::homePath() + "/" +
                                      QString(APPLICATION_SETTINGS_FILENAME);
            QSettings settings(settingsFName,QSettings::IniFormat);

            settings.setValue(SETS_POINTER_PENNAME,pointerPenName->text());
            settings.setValue(SETS_POINTER_PASSWORD,passwordEntry->text());

            settings.setValue(SETS_HOSTNAME,hostEdit->text());
            settings.setValue(SETS_HOSTPORT,portEdit->text());
            settings.setValue(SETS_HOSTVIEWNAME,hostViewEdit->text());
            settings.setValue(SETS_HOSTVIEWPORT,viewPortEdit->text());
            settings.setValue(SETS_HAS_HOSTVIEW,remoteCustomConnect->isChecked());
        }

        // Remove the dialogue (now that we've retreived the data from the dialogue)
        hide();
#if 0
        emit connectedToRemoteHost();
    }
    else
    {
        emit failedRemoteConnection();
    }
#endif
}

bool remoteConnectDialogue::connectPenToHost(void)
{
    bool          answered = false;
    net_message_t request;
    net_message_t data;
    net_message_t resp;
    QHostAddress  respHost;
    quint16       respPort;

    qDebug() << "Requesting to join as a pen. Colour:" <<
                data.msg.join.colour[0] << data.msg.join.colour[1] << data.msg.join.colour[2];

    request.messageType      = NET_JOIN_REQ_PACKET;
    request.protocolVersion  = PROTOCOL_VERSION;
    request.messageTypeCheck = NET_JOIN_REQ_PACKET;

    // May want to remove the "toLocal8Bit" for internationalisation in the future.
    memcpy(request.msg.join.pen_name, penCStringName, MAX_USER_NAME_SZ-1);
    request.msg.join.pen_name[MAX_USER_NAME_SZ-1] = (char)0;

    request.msg.join.colour[0] = colour.red();
    request.msg.join.colour[1] = colour.green();
    request.msg.join.colour[2] = colour.blue();

    QString pwd = passwordEntry->text().trimmed();

    if( pwd.length()>0 )
    {
        passwordBA.resize(KEY_LENGTH);
        passwordBA.fill(0);

        // Get a fixed length password. If password is longer than key length, wrap and xor onto
        // previous data. If shorter, repeat the password characters.
        QByteArray pwdChar = pwd.toLocal8Bit();

        for(int i=0; i<pwd.length() && i<KEY_LENGTH; i++)
        {
            passwordBA[i % KEY_LENGTH] = passwordBA.at(i % KEY_LENGTH) ^ pwdChar[i % KEY_LENGTH];
        }

        // More muddling.
        for(int i=1; i<KEY_LENGTH; i++)
            passwordBA[i] = passwordBA.at(i) + passwordBA.at(i-1);

        // Set the flag
        passwordIsNotNull = true;

        // Encrypt outgoing message
        passwordEncryptMessage(&data);
    }
    else
    {
        passwordIsNotNull = false;
        memcpy(&data, &request, sizeof(data));
    }

    remoteConnection = false;

    int numTries = 0;
    int bytesIn  = 0;

    while( ! answered && numTries<10 )
    {
        // Encrypt (we redo this every time so it can encrypt differently on each send)
        if( passwordIsNotNull )
        {
            memcpy(&data, &request, sizeof(data));
            passwordEncryptMessage(&data);
        }

        // Send request to join to sysTray
        remoteSocket->writeDatagram((char *)&data,sizeof(data),hostAddr,hostPortNum);
        remoteSocket->waitForBytesWritten();

        // wait for reply (for up to 200ms)
        if( remoteSocket->waitForReadyRead(200) )
        {
            bytesIn = remoteSocket->readDatagram((char *)&resp,sizeof(resp),&respHost,&respPort);

            if( bytesIn > 0 && respHost == hostAddr )
            {
                if( passwordIsNotNull )
                {
                    passwordDecryptMessage( &resp );
                }

                if( resp.messageType == NET_JOIN_ACCEPT )
                {
                    // Set up join
                    remoteConnection = true;
                    remotePenNum     = resp.msg.join.pen_num;
                    answered         = true;

                    qDebug() << " Pen num on remote system:" << remotePenNum;
                }
                else if( resp.messageType == NET_JOIN_REJECT )
                {
                    // Failed to connect
                    remoteConnection = false;
                    answered         = true;

                    qDebug() << "Join request refused by host.";
                }
                else
                {
                    qDebug() << "Unexpected" << bytesIn << "bytes in message of type:" << (0+resp.messageType);
                    qDebug() << "Incomming message:" << QByteArray(((char *)&resp),bytesIn).toHex();
                }

            } else qDebug() << "Rx " << bytesIn << "bytes from" << respHost << "port" << respPort;
        }
        else qDebug() << "No response from host.";

        numTries ++;
    }

    return remoteConnection;
}

bool remoteConnectDialogue::disconnectPenFromHost(void)
{
    net_message_t request;
    request.messageType      = NET_PEN_LEAVE_PACKET;
    request.protocolVersion  = PROTOCOL_VERSION;
    request.messageTypeCheck = NET_PEN_LEAVE_PACKET;
    net_message_t resp;
    QHostAddress  respHost;
    quint16       respPort;

    int  numTries = 0;

    while( numTries<10 )
    {
        // Send request to join to sysTray
        remoteSocket->writeDatagram((char *)&request,sizeof(net_message_t),hostAddr,hostPortNum);
        remoteSocket->waitForBytesWritten();

        // wait for reply (for up to 200ms)
        if( remoteSocket->waitForReadyRead(200) )
        {
            int bytesIn = remoteSocket->readDatagram((char *)&resp,sizeof(resp),&respHost,&respPort);

            if( bytesIn > 0 && respHost == hostAddr )
            {
                if( resp.messageType == NET_PEN_LEAVE_ACK && resp.msg.join.pen_num == remotePenNum )
                {
                    qDebug() << "Disconnect ACKnowledged";

                    return true;
                }
                else qDebug() << "Not ack type:" << resp.messageType << "penNum:" << remotePenNum;
            }
            else qDebug() << "Bad size:" << bytesIn << "or source:" << respHost;
        }

        numTries ++;
    }

    return false;
}

int remoteConnectDialogue::penNumberOnHost(void)
{
    return remotePenNum;
}


// Temp: just encrypt by xoring with key

static int saltChange = 1;

void remoteConnectDialogue::passwordEncryptMessage(net_message_t *msg)
{
    if( passwordIsNotNull )
    {
        msg->salt = saltChange;
        saltChange ++;

        char *key = passwordBA.data();
        char *dat = (char *)msg;

        for(int i=0; i<sizeof(net_message_t); i++)
        {
            dat[i] ^= key[i % KEY_LENGTH];
        }
    }
}

void remoteConnectDialogue::passwordDecryptMessage(net_message_t *msg)
{
    if( passwordIsNotNull )
    {
        saltChange ++;

        char *key = passwordBA.data();
        char *dat = (char *)msg;

        for(int i=0; i<sizeof(net_message_t); i++)
        {
            dat[i] ^= key[i % KEY_LENGTH];
        }
    }
}

void remoteConnectDialogue::decryptNextWord(int &index, quint8 encryptedWord[4])
{
    if( passwordIsNotNull )
    {
        char *key = passwordBA.data() + index;

        for(int i=0; i<4; i++)
        {
            encryptedWord[i] ^= key[i % KEY_LENGTH];
        }

        index += 4;
        if( index >= KEY_LENGTH ) index = 0;
    }
}


void remoteConnectDialogue::remoteCustomEnableChanged()
{
    if( remoteCustomConnect->isChecked() )
    {
        hostViewEdit->setEnabled(true);
        viewPortEdit->setEnabled(true);
    }
    else
    {
        hostViewEdit->setEnabled(false);
        viewPortEdit->setEnabled(false);

        hostViewEdit->setText( hostEdit->text() );
        viewPortEdit->setText( QString("%1").arg(1+portEdit->text().toInt()) );
    }
}

void remoteConnectDialogue::rejectRemoteConnect()
{
    hide();
}

void remoteConnectDialogue::sendDebugSelected()
{
    sendDebugButton->setEnabled(false);

    emit sendDebugRequested();
}

void remoteConnectDialogue::quitProgramSelected()
{
    hide();

    emit connectDialogueQuitProgram();
}

void remoteConnectDialogue::helpSelected()
{
    emit connectDialogueHelp();
}

QHostAddress remoteConnectDialogue::hostViewAddress(void)
{
    return hostViewAddr;
}

int remoteConnectDialogue::hostViewPort(void)
{
    return hostViewPortNum;
}

QHostAddress remoteConnectDialogue::hostAddress(void)
{
    return hostAddr;
}

int remoteConnectDialogue::hostPort(void)
{
    return hostPortNum;
}


// /////
// RADAR
// /////

void remoteConnectDialogue::pingForServers()
{
//    qDebug() << "Ping for servers...";

    // Send a radar ping... and listen for the echos
    radarSocket.writeDatagram((char *)&dataPacket,sizeof(dataPacket),
                              QHostAddress::Broadcast, 14755);

    // Timeout any servers that we haven't heard from for a while
    // Give 'em 20 seconds as we don't ping very often and packets will be lost.

    for(int index=(foundServersList.length()-1); index>=0; index --)
    {
        // Timeout after 20 seconds
        if( foundServersList[index].lastResponseTime.secsTo(QTime::currentTime()) > 20 )
        {
            qDebug() << "Remove Radar server entry" << index;

            // Timeout: delete this sever entry
            foundServersList.remove(index);
            foundServersCombo->removeItem(index);
        }
    }

    if( foundServersList.length() < 1 )
    {
        foundServersCombo->hide();
        manualDataButton->show();
        searchingForSessions->show();
        adjustSize();
    }

    // Keep polling while this dialogue is visible
    if( isVisible() )
    {
        int delay = 2000 + (qrand() % 1000);

        QTimer::singleShot(delay, this, SLOT(pingForServers()));
    }
    else
    {
        // Stop the socket
//        radarSocket.close();

        // Stop the RADAR (0 => all slots)
//        disconnect(&radarSocket, SIGNAL(readyRead()), 0, 0);
    }
}

void remoteConnectDialogue::manualConnectData()
{
    // User has selected manual configuration
    userSelectedManualConfig = true;

    manualConfigArea->show();
    connectButton->show();
}

void remoteConnectDialogue::serverResponded()
{
    // Extract the packet
    int            incommingSz = radarSocket.pendingDatagramSize();

    quint16        rxPort;
    char          *rxBuffer  = (char *)calloc(incommingSz,sizeof(char));
    net_message_t *rxMessage = (net_message_t *)rxBuffer;
    QHostAddress   rxAddress;

    if( radarSocket.readDatagram(rxBuffer,incommingSz,&rxAddress,&rxPort) == sizeof(net_message_t))
    {
        if( rxMessage->messageType == NET_RADAR_ECHO )
        {
            // Is there a list entry for this already? In range => found (will be off end if not)
            int  index;

            for(index=0; index<foundServersList.length(); index ++)
            {
                if( foundServersList[index].IPAddress == rxAddress )
                {
                    break;
                }
            }

            // Just in case...
            rxMessage->msg.radar.session_name[SESSION_NAME_SZ-1] = (char)0;

            QString serverNameStr = QString(rxMessage->msg.radar.session_name);

            if( index>=foundServersList.length() )
            {
                qDebug() << "Add a server:" <<serverNameStr;

                // New entry

                serverEntry_t newServer;

                newServer.IPAddress        = rxAddress;
                newServer.serverName       = serverNameStr;
                newServer.lastResponseTime = QTime::currentTime();

                // NB We want both lists to match in terms of index
                foundServersList.append(newServer);

                qDebug() << "Append new server to combo box at index" << index;

                // Without the disconnect, we automatically move to the new item. :(
                disconnect(foundServersCombo,SIGNAL(currentIndexChanged(int)), 0, 0);
                foundServersCombo->insertItem(index,serverNameStr);
                connect(foundServersCombo,SIGNAL(currentIndexChanged(int)), this, SLOT(foundServerSelected(int)));

                if( ! userSelectedManualConfig && foundServersList.length() == 1 )
                {
                    hide();
                    foundServersCombo->setCurrentIndex(0);
                    show();
#ifdef Q_OS_ANDROID
                   // Manually center on parent as it isn't done automatically. Sigh.
                    QPoint topLeft( parentWidth/2  - width()/2,
                                    parentHeight/2 - height()/2);

                    move(topLeft);
#endif
                }

                repaint();
            }
            else
            {
                // Entry already exists

                foundServersList[index].serverName       = serverNameStr;
                foundServersList[index].lastResponseTime = QTime::currentTime();
            }

            connectButton->show();
            foundServersCombo->show();
            manualDataButton->hide();
            searchingForSessions->hide();

            adjustSize();
        }
    }

    // Stop receiving if we're not looking for a host
    if( ! isVisible() )
    {
        // Stop the RADAR (0 => all slots)
        disconnect(&radarSocket, SIGNAL(readyRead()), 0, 0);
    }

    free( rxBuffer );
}


void remoteConnectDialogue::foundServerSelected(int newIndex)
{
    qDebug() << "foundServerSelected:" << newIndex;

    if( newIndex >= foundServersList.length() )
    {
        // User has selected manual configuration
        userSelectedManualConfig = true;

        manualConfigArea->show();
        connectButton->show();
    }
    else
    {
        // User wants to use discovered host
        manualConfigArea->hide();
    }
    hide();
    show();
#ifdef Q_OS_ANDROID
   // Manually center on parent as it isn't done automatically. Sigh.
    QPoint topLeft( parentWidth/2  - width()/2,
                    parentHeight/2 - height()/2);

    move(topLeft);
#endif
}

