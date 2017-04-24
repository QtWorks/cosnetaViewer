#ifndef VIEWSERVERLISTENER_H
#define VIEWSERVERLISTENER_H

#include <QTcpServer>

class viewServerListener : public QTcpServer
{
    Q_OBJECT
public:
    explicit viewServerListener(int newPort, QObject *parent = 0);
    void     startServer();
    
signals:
    void     connectionStarted(qintptr socketID);
    
public slots:

protected:
    void  incomingConnection(qintptr socketDesc);

private:
    int port;
    
};

#endif // VIEWSERVERLISTENER_H
