#include "viewServerListener.h"

viewServerListener::viewServerListener(int newPort, QObject *parent) : QTcpServer(parent)
{
    port = newPort;
}

void viewServerListener::startServer()
{
    if( ! this->listen(QHostAddress::Any, port) )
    {
        qDebug() << "Failed to start view server listening";
    }
    else
    {
        qDebug() << "Started view server listening.";
    }
}


// This is the only reason for the existance of this class. In order to
// get a socketDescriptor that can be used in another thread we need
// to be a subclass of QTcpServer which cannot be done (not supported)
// in a widget as they both would eventually become subclassed of
// QObject which doesn't work.

void viewServerListener::incomingConnection(qintptr socketDesc)
{
    qDebug() << "viewServerListener::incomingConnection ID =" << socketDesc;

    emit connectionStarted(socketDesc);
}

