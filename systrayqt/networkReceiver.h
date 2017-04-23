#ifndef NETWORKRECEIVER_H
#define NETWORKRECEIVER_H

#include <QObject>
#include <QThread>
#include <QString>
#include <QtNetwork>

#include "main.h"
#include "FreerunnerReader.h"


#define MAX_IP_CNX 8


#define USE_UDP

class networkReceiver : public QObject
{
    Q_OBJECT

public:
    explicit networkReceiver(sysTray_devices_state_t *table, QObject *parent = 0);
    ~networkReceiver();
    
    void    doConnect(QThread &cThread);
    void    stopCommand(void);
    void    startCommand(void);
    void    waitForStop(void);
    void    setNewPassword(QString newPassword);
    void    passwordEncryptMessage(net_message_t *msg);
    void    passwordDecryptMessage(net_message_t *msg);
    void    connectionStatus(QString &hostIP, int &port, QString &status);
    void    updateScreenSize(int w, int h);
    bool    tableEntryIsNetworkConnection(int tableIndex);
    bool    setMaxNumIPConnections(int maxConnections);
    int     getMaxNumIPConnections(void);

public slots:
    void    readThread();

private:
#ifdef USE_UDP
    QString listIPAddresses(void);
#else
    void    updateFromSocketConnection(QTcpSocket *cnx);
#endif

    sysTray_devices_state_t *sharedTable;
    QNetworkInterface       *netIface;

    int                      portNum;
    QString                  ipAddress;
    QString                  state;
    int                      maxNumActiveSenders;
    int                      numSendersActive;
    int                      screenWidth;
    int                      screenHeight;
    bool                     senderActive[MAX_IP_CNX];
    int                      senderTarget[MAX_IP_CNX];
    bool                     haveSessionKey;
    QByteArray               sessionKey;

};

#endif // NETWORKRECEIVER_H
