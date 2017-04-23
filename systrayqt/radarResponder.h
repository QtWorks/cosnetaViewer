#ifndef RADARRESPONDER_H
#define RADARRESPONDER_H

#include <QObject>
#include <QThread>
#include <QUdpSocket>

#include "savedSettings.h"

class radarResponder : public QObject
{
    Q_OBJECT
public:
    explicit radarResponder(savedSettings *settingRef, QObject *parent = 0);
    ~radarResponder();

    void     doConnect(QThread &cThread);
    void     stopCommand(void);

signals:

public slots:
    void    answerThread();

private:
    void           getMulticastConnection(void);

    QUdpSocket    *socket;
    savedSettings *settings;
    bool           quitResponder;
};

#endif // RADARRESPONDER_H
