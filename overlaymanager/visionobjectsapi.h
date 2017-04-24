#ifndef VISIONOBJECTSAPI_H
#define VISIONOBJECTSAPI_H

#include <QObject>
#include <QtNetwork/QtNetwork>

class VisionObjectsAPI : public QObject
{
    Q_OBJECT

public:
    explicit VisionObjectsAPI(QObject *parent = 0);
    void getRecognitionResult(QVector<QVector<QPoint> > strokePoints);

private:
    void postRequest( const QString &url, const QByteArray &encodedMessageTextString );
    QString createJson(QVector<QVector<QPoint> > strokePoints);
    QString addJsonHeader();
    QString addJsonFooter();
    void delay(int delaySecs);

signals:
    void resultReady( const QByteArray &resultAsJSON );

public slots:
    void parseNetworkResponse( QNetworkReply *finished );

private:
    QNetworkAccessManager m_nam;
    QByteArray postData;
};

#endif // VISIONOBJECTSAPI_H
