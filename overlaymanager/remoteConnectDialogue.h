#ifndef REMOTECONNECTDIALOGUE_H
#define REMOTECONNECTDIALOGUE_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QColor>
#include <QUdpSocket>
#include <QHostAddress>
#include <QTime>
#include <QVector>
#include <QEvent>

#include "cosnetaAPI.h"
#include "../systrayqt/net_connections.h"

typedef struct server_s
{
    QString      serverName;
    QHostAddress IPAddress;
    QTime        lastResponseTime;

} serverEntry_t;

class remoteConnectDialogue : public QWidget
{
    Q_OBJECT
public:
    explicit remoteConnectDialogue(QUdpSocket *skt, QWidget *parent = 0);

    void         startRemoteConnection(bool penIsMouse, char* penName, QColor penColour, int newParentHeight, int newParentWidth);
    void         reStartRemoteConnection(int newParentHeight, int newParentWidth);
    void         setStatusString(QString newStatus);
    int          penNumOnRemoteSystem(void);
    QHostAddress hostAddress(void);
    int          hostPort(void);
    QHostAddress hostViewAddress(void);
    int          hostViewPort(void);
    bool         havePassword(void);
    void         decryptNextWord(int &index, quint8 encryptedWord[4]); // for stream use
    void         passwordEncryptMessage(net_message_t *msg);
    void         passwordDecryptMessage(net_message_t *msg);
    int          penNumberOnHost(void);
    bool         connectPenToHost(void);
    bool         disconnectPenFromHost(void);


signals:
    void     failedRemoteConnection();
    void     connectedToRemoteHost();
    void     remoteHostSelected();
    void     connectDialogueQuitProgram();
    void     connectDialogueHelp();
    void     sendDebugRequested();

protected:
#ifdef Q_OS_ANDROID
    bool       event(QEvent *e);
#endif

public slots:
    void       remoteCustomEnableChanged();
//    void       rejectRemoteConnectSelected();

private slots:
    void        foundServerSelected(int newIndex);
    void        remoteConnect();
    void        rejectRemoteConnect();
    void        sendDebugSelected();
    void        quitProgramSelected();
    void        helpSelected();
    void        pingForServers();
    void        serverResponded();
    void        manualConnectData();

private:
    QLabel         *remoteConnectDialogueStatus;
    QWidget        *manualConfigArea;
    QLabel         *searchingForSessions;
    QLineEdit      *pointerPenName;
    QLineEdit      *passwordEntry;
    QString         initialPenName;
    QComboBox      *foundServersCombo;
    QLineEdit      *hostEdit;
    QLineEdit      *portEdit;
    QLineEdit      *hostViewEdit;
    QLineEdit      *viewPortEdit;
    QCheckBox      *remoteCustomConnect;
    QPushButton    *connectButton;
    QPushButton    *manualDataButton;
    QPushButton    *sendDebugButton;

    pen_settings_t *penSettings;
    QColor          colour;
    bool            penIsLocalPointer;
    QByteArray      passwordBA;
    bool            passwordIsNotNull;
    char            penCStringName[MAX_USER_NAME_SZ+1];

    remoteConnectDialogue *decryptor;

    int             parentWidth;
    int             parentHeight;

    QUdpSocket     *remoteSocket;
    QUdpSocket      radarSocket;
    QHostAddress    hostAddr;
    int             hostPortNum;
    net_message_t   dataPacket;

    // Server list data
    QVector<serverEntry_t> foundServersList;

    bool            userSelectedManualConfig;

    // Response data (can be queried by caller)
    int             remotePenNum;
    bool            remoteConnection;
    QHostAddress    hostViewAddr;
    int             hostViewPortNum;

};

#endif // REMOTECONNECTDIALOGUE_H
