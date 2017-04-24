#include "visionobjectsapi.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QFile>
#include <QDebug>

#define URL "https://myscript-webservices.visionobjects.com/api/myscript/v2.0/analyzer/doSimpleRecognition.json"
#define API_KEY "f389e28f-5ac1-44ae-a094-1420d99ddaf6"
#define DEBUG 1

VisionObjectsAPI::VisionObjectsAPI(QObject *parent) :
    QObject(parent)
{    
    QObject::connect(&m_nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(parseNetworkResponse(QNetworkReply*)));
}

QString VisionObjectsAPI::createJson(QVector<QVector<QPoint> > strokePoints) {

    QString rtnString;
    QString dq = "\"";
    QString typeStroke, xPre, xPost, yPre, yEndStrokes, yPost, yClose, jsonClose;
    QString jsonString;
    QString strokeString;

    // Setup the strings
    typeStroke.append("{");
    typeStroke.append(dq);
    typeStroke.append("type");
    typeStroke.append(dq);
    typeStroke.append(":");
    typeStroke.append(dq);
    typeStroke.append("stroke");
    typeStroke.append(dq);
    typeStroke.append(",");
    xPre.append(dq);
    xPre.append("x");
    xPre.append(dq);
    xPre.append(":");
    xPre.append("[");
    xPost.append("],");
    yPre.append(dq);
    yPre.append("y");
    yPre.append(dq);
    yPre.append(":");
    yPre.append("[");
    yPost.append("]},");
    yEndStrokes.append("]}");
    yClose.append("]}");
    jsonClose.append("]}");

    // Render strokepoints into the form VO expects
    // Iterate over all the strokes first
    for(int s=0; s<strokePoints.size(); s++)
    {
        // Set up the stroke header
        strokeString.clear();
        strokeString.append(typeStroke);

        // Process x coords
        strokeString.append(xPre);
        QString x_str, y_str;
        int x_int, y_int;
        for(int p=0; p<strokePoints[s].size(); p++)
        {
            // List out the x coords of each vector first
            x_int = strokePoints[s][p].x();
            x_str = QString::number(x_int);
            strokeString.append(x_str);
            if (p<strokePoints[s].size()-1)
            {
                strokeString.append(",");
            }
        }
        strokeString.append(xPost);

        // Process y coords
        strokeString.append(yPre);
        for(int p=0; p<strokePoints[s].size(); p++)
        {
            // List out the y coords of each vector first
            y_int = strokePoints[s][p].y();
            y_str = QString::number(y_int);
            strokeString.append(y_str);
            if (p<strokePoints[s].size()-1)
            {
                strokeString.append(",");
            }
        }
        if (s < strokePoints.size()-1 )
        {
            strokeString.append(yPost);
        }
        else
        {
            strokeString.append(yClose);
        }
        jsonString.append(strokeString);
    }
    rtnString.append(jsonString);

    // Now clear strokePoints ready for the next stroke
    strokePoints.clear();

    return rtnString;
}

QString VisionObjectsAPI::addJsonHeader() {

    QString dq = "\"";
    QString jsonHeader;

    jsonHeader.append("{");
    jsonHeader.append(dq);
    jsonHeader.append("parameter");
    jsonHeader.append(dq);
    jsonHeader.append(": {");
    jsonHeader.append(dq);
    jsonHeader.append("hwrParameter");
    jsonHeader.append(dq);
    jsonHeader.append(": {");
    jsonHeader.append(dq);
    jsonHeader.append("language");
    jsonHeader.append(dq);
    jsonHeader.append(":");
    jsonHeader.append(dq);
    jsonHeader.append("en_US");
    jsonHeader.append(dq);
    jsonHeader.append("} } ,");
    jsonHeader.append(dq);
    jsonHeader.append("components");
    jsonHeader.append(dq);
    jsonHeader.append(": [");

    return jsonHeader;
}

QString VisionObjectsAPI::addJsonFooter() {

    QString jsonFooter;
    jsonFooter.append("]}");

    return jsonFooter;
}

void VisionObjectsAPI::getRecognitionResult(QVector<QVector<QPoint> > strokePoints)
{
    QString inputJson, sendJson;
    QByteArray jsonData;

    sendJson.append(addJsonHeader());
    inputJson = createJson(strokePoints);
    sendJson.append(inputJson);
    sendJson.append(addJsonFooter());

    //qDebug() << "Sending this json to Vision Objects:\n" << sendJson;

    jsonData.append(sendJson);

    QJsonParseError *err = new QJsonParseError();

    QJsonDocument doc = QJsonDocument::fromJson(jsonData, err);

    if (err->error != 0)
    {
        qDebug() << "QJsonParseError is " << err->errorString();
    }
    else if (inputJson.size() == 72 ) // This is the length of the "empty" string {"parameter": {"hwrParameter": {"language":"en_US"} } ,"components": []}
    {
        qDebug() << "No json data returned";
    }
    else
    {
        // Remove the newline character from jsonData
        QString jsonDataString = jsonData;
        QByteArray jsonDataSimplified;
        jsonDataSimplified.append(jsonDataString.simplified());

        QByteArray introUrl = "apiKey=f389e28f-5ac1-44ae-a094-1420d99ddaf6&analyzerInput=";
        QByteArray messageText = QByteArray(jsonDataSimplified);
        QByteArray encodedMessageText( messageText.toPercentEncoding( ) ) ;
        postData
            .append(introUrl)
            .append(encodedMessageText);

        // Output the JSON to a file so I can test the service manually from my browser
        /**
        QFile jsonFileOut("C:\\Users\\elekt_000\\Desktop\\json_being_sent_from_freestyle.json");
        if (!jsonFileOut.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            qDebug() << "Unable to open json_being_sent_from_freestyle.json, exiting...";
            exit(1);
        }
        QTextStream out(&jsonFileOut);
        out << postData;
        jsonFileOut.close();
        **/

        postRequest(URL, postData);

        //Clear postData so we don't get repitition
        postData.clear();
        sendJson.clear();
        inputJson.clear();
    }
}

void VisionObjectsAPI::postRequest( const QString &urlString, const QByteArray &postDataToUrl )
{
    QUrl url ( urlString );
    QNetworkRequest req ( url );
    req.setHeader( QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded" );
    m_nam.post(req, postDataToUrl);

    qDebug() << "Request posted to " << urlString;
    // This bit needs some work - perhaps create a ping-like test of delay
    // VO web service response time from Alba (Ethernet not WiFi) is 2 secs
    // VO web service response time from my home via WiFi is ca. 3 secs
    delay(3);
}

void VisionObjectsAPI::parseNetworkResponse( QNetworkReply *finished )
{
    // Reading attributes of the reply
    QVariant statusCodeV = finished->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    qDebug() << QString("HTTP response code: " + statusCodeV.toString());

    // "200 OK" received?
    QByteArray bytes;
    if (statusCodeV.toInt()==200)
    {
        bytes = finished->readAll();  // bytes
        const QString string(bytes); // string
        qDebug() << QString("Response from Vision Objects service is:\n" + string);
    }
    // Some http error or redirect
    else
    {
        qDebug() << QString("Failed.");
    }

    emit resultReady( bytes );
    delete finished, bytes;
}

void VisionObjectsAPI::delay(int delaySecs)
{
    QTime dieTime= QTime::currentTime().addSecs(delaySecs);
    while( QTime::currentTime() < dieTime )
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}
