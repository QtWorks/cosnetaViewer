#include <QColor>
#include <QPainter>
#include <QFileDialog>
#include <QDir>
#include <QFile>

#include <QApplication>
#include <QDesktopWidget>
#include <QImage>
#include <QPixmap>
#include <QBuffer>
#include <QTcpServer>

#include <QDebug>

#include <math.h>

#include "overlay.h"
#include "penCursor.h"

//#define DEBUG 1
#include <iostream>


// A bit nasty, but we may want to store a lot of data for a long session
#define ACTION_DELETE(x)     ((x)  | 0x80000000)
#define ACTION_IS_DELETED(x) (((x) & 0x80000000)==0x80000000)
#define ACTION_UNDELETE(x)   ((x)  & 0x7FFFFFFF)

#ifdef DEBUG
#include <iostream>
#endif

overlay::overlay(QImage  *image,
                 QImage  *hlights,
                 QImage  *textrender,
                 QColor  *initColour, QColor inputBackgroundColour,
                 bool     canSend,
                 screenGrabber *grabTask,
                 QWidget *parent) : QWidget(parent)
{
    int        p;

    annotations    = image;
    highlights     = hlights;
    textLayerImage = textrender;

    w = annotations->width();
    h = annotations->height();

    for(p=0; p<MAX_SCREEN_PENS; p++)
    {
        defaultColour[p] = initColour[p];
    }
    defaultBackgroundColour = inputBackgroundColour;

    // Font for the text overlay
    defaultFont = new QFont("courier");

    if( ! defaultFont->fixedPitch() )
    {
        defaultFont->setFixedPitch(true);

        if( ! defaultFont->fixedPitch() )
        {
            defaultFont->setFamily("monospace");
        }
    }
    defaultFont->setPointSize(12);

    currentpageNum = 0;

    pageData_t first;

    for(p=0; p<MAX_SCREEN_PENS; p++) first.undo[p].active = false; // Just to be sure
    page.push_back(first);

    setDefaults();  // Sets defaults for currentPageNum

    // And create a TCP server which will call a local slot when a connection occurs.
    canBeServer = canSend;
#ifndef TABLET_MODE
    server.clear();
    if( canSend )
    {
        listener = new viewServerListener(DEFAULT_HOST_VIEW_PORT);
        connect(listener, SIGNAL(connectionStarted(qintptr)), this, SLOT(incomingConnection(qintptr)) );

        listener->startServer();
    }
    else
    {
        listener = NULL;
    }

    screenGrabTask = grabTask;
#endif
    parentWindow   = parent;
}

overlay::~overlay()
{
    delete defaultFont;
}


void overlay::incomingConnection(qintptr cnxID)
{
    qDebug() << "View socket connecting. connectionID =" << cnxID;

    if( ! canBeServer ) return;

#ifndef TABLET_MODE

    // Create the child socket from the incomming connection.
    ViewServer *sender = new ViewServer(cnxID, screenGrabTask);

    connect(sender,SIGNAL(IAmDead()), this, SLOT(checkForDeadSenders()));

    sender->start(); // From here, data will be sent. Not locked on startup

    sender->waitForInitComplete();

    // Find this device in the list of connected devices to check whether to send
    // the display to it (or just to send pen state changes for menu updates).
#if 1
    bool senderWantsView = true;
#else
    char penIPAddrStr[NET_PEN_ADDRESS_SZ];
    bool senderWantsView = false;

    for( int p=0; p<MAX_PENS; p++ )
    {
        if( cos_get_net_pen_addr(p, penIPAddrStr) )
        {
            if( sender->connectedHostName() == QString(penIP))
            {
                senderWantsView = COS_PEN_CAN_VIEW();
            }
        }
    }
#endif
    // If its a phone, tell the server to not send any screen image data
    // (the default is that it does)

    if( ! senderWantsView )
    {
        sender->clientDoesNotWantView();
    }

    // Send our display dimensions

    QByteArray *message = new QByteArray(8,0);

    message->data()[0] = ACTION_SCREENSIZE;
    message->data()[4] = (w >> 8);
    message->data()[5] =  w;
    message->data()[6] = (h >> 8);
    message->data()[7] =  h;

    sender->queueData(message);

    // Send current page state
    sendCurrentPageState(sender, currentpageNum);

    if( senderWantsView )
    {
        // If the background is transparent, send the background image
//        if( page[currentpageNum].currentBackgroundColour.alpha() < 255 )
        {
    //        parentWindow->hide();

            // Grab a background image
            QPixmap background = screenGrabTask->currentBackground(); // QPixmap::grabWindow( QApplication::desktop()->winId() );

            // JPEG compression needs to be multiples of 8x8
            if( (background.width() & 7) || (background.height() & 7) )
            {
                QPixmap croppedImage = background.copy(0, 0,
                                                       (background.width()  & ~7),
                                                       (background.height() & ~7));
                background = croppedImage;
            }

    //        parentWindow->show();

            // Now send it...
            QByteArray *bytes = new QByteArray;
            QBuffer     buffer(bytes);
            buffer.open(QIODevice::WriteOnly);
            background.save(&buffer, "JPG", 4);

            int   imageSize = bytes->size();

            // Minimum valid image size (I've been seeing zeros)
            if( imageSize < 134 )
            {
                qDebug() << "Got invalid image size:" << imageSize << "from background:" << background;
            }
            else
            {
                switch( imageSize & 3 )
                {
                case 1:  bytes->append((char)0);
                case 2:  bytes->append((char)0);
                case 3:  bytes->append((char)0);
                }

                qDebug() << "Send JPG background image to new client. Size =" << imageSize;

                bytes->insert(0,"1234",4);
                bytes->data()[0] = ACTION_BACKGROUND_JPG;
                bytes->data()[1] = ( imageSize >> 16 ) & 255;
                bytes->data()[2] = ( imageSize >>  8 ) & 255;
                bytes->data()[3] = ( imageSize       ) & 255;

                // and send it to the new client
                sender->queueData(bytes);
            }
        }
//        else
//        {
//            qDebug() << "Don't send background image as background is opaque.";
//        }
    }

    sender->queueIsNowPreloaded();

    qDebug() << "Start sender:" << sender;

    // From here, we will send 'current' updates
    server.append(sender);

    qDebug() << "New server started for client connection" << cnxID;
#endif
}

void  overlay::checkForDeadSenders(void)
{
    qDebug() << "checkForDeadSenders.";
#ifndef TABLET_MODE
    QList<ViewServer *> removeList;

    for(int i=(server.size()-1); i>=0; i--)
    {
        // Is it waiting to die?
        if( server[i]->waitingToDie() )
        {
            // Remove the pointer from the server list so no more data is sent
            // while adding the pointer to the remove list.
            removeList.append( server.takeAt(i) );
        }
    }

    // Allow other threads to run (in case we have threading issues)

    QThread::yieldCurrentThread();
    qDebug() << "Unblock pending servers.";

    // Unblock any pending threads

    for(int i=0; i<removeList.size(); i++)
    {
        removeList[i]->youMayNowDie();
    }

    // Allow other threads to run (in case we have threading issues)

    QThread::yieldCurrentThread();
    qDebug() << "Now delete pending servers.";

    // Destroy the items on the queue
#if 0
    for(int i=0; i<removeList.size(); i++)
    {
        while( removeList[i]->hasBeenStopped == false )
        {
            qDebug() << "Delete pending server... waiting.";

            QThread::yieldCurrentThread();
        }

        delete removeList[i];
    }
#endif
#endif
}


void overlay::sendCurrentPageState(ViewServer *sender, int pageNum)
{
#ifndef TABLET_MODE
    // Let's not sendCurrentPageState if there are no clients.
    if( ! canBeServer ) return;

    // Send the background colour

    QByteArray *message = new QByteArray(4*(2+MAX_SCREEN_PENS*(1+1+2)),0);

    message->data()[0] = ACTION_BACKGROUND_COLOUR;
    message->data()[4] = page[currentpageNum].currentBackgroundColour.red();
    message->data()[5] = page[currentpageNum].currentBackgroundColour.green();
    message->data()[6] = page[currentpageNum].currentBackgroundColour.blue();
    message->data()[7] = page[currentpageNum].currentBackgroundColour.alpha();

    QColor col;

    // Send current pen states (page[pageNum].currentPens)
    for(int penNum=0; penNum<MAX_SCREEN_PENS; penNum++)
    {
        message->data()[8 + 0  + penNum*4*(1+1+2)] = ACTION_PENTYPE;
        message->data()[8 + 2  + penNum*4*(1+1+2)] = page[pageNum].startPenState[penNum].drawStyle;
        message->data()[8 + 3  + penNum*4*(1+1+2)] = penNum;

        message->data()[8 + 4  + penNum*4*(1+1+2)] = ACTION_PENWIDTH;
        message->data()[8 + 6  + penNum*4*(1+1+2)] = page[pageNum].startPenState[penNum].thickness;
        message->data()[8 + 7  + penNum*4*(1+1+2)] = penNum;

        message->data()[8 + 8  + penNum*4*(1+1+2)] = ACTION_PENCOLOUR;
        message->data()[8 + 11 + penNum*4*(1+1+2)] = penNum;

        col  = page[pageNum].startPenState[penNum].colour;

        message->data()[8 + 12 + penNum*4*(1+1+2)] = col.red();
        message->data()[8 + 13 + penNum*4*(1+1+2)] = col.green();
        message->data()[8 + 14 + penNum*4*(1+1+2)] = col.blue();
        message->data()[8 + 15 + penNum*4*(1+1+2)] = col.alpha();
    }
#ifdef SEND_ACTIONS_TO_CLIENTS
    // Send the current overlay
    for(int actionNum = 0; actionNum < page[pageNum].previousActions.size(); actionNum++)
    {
        // NB This allActions will account for UNDO/REDO etc
        sender->queue32bitData(page[pageNum].allActions[actionNum]);
    }
#endif
    sender->queueData(message);
#endif
}

void overlay::updateClientsOnSessionState(int sessionType)
{
#ifndef TABLET_MODE
    // Let's not sendCurrentPageState if there are no clients.
    if( ! canBeServer ) return;

    QByteArray data = QByteArray(4,0);
    data[0] = ACTION_SESSION_STATE;
    data[1] = sessionType >> 8;
    data[2] = sessionType & 255;

    qDebug() << "Update clients of new session state. Message:" << QString("0x%1").arg(data.constData(),0,16);

    if( server.size() > 0 )
    {
        QList<ViewServer *>::Iterator it = server.begin();

        while( it != server.end())
        {
            QByteArray *dataCopy = new QByteArray(data);

            (*it)->queueData(dataCopy);

            // Next
            ++it;
        }
    }
#endif
}

void overlay::updateClientsOnPenRole(int penNum, penRole_t newRole)
{
#ifndef TABLET_MODE
    // Let's not sendCurrentPageState if there are no clients.
    if( ! canBeServer ) return;

    QByteArray data = QByteArray(4,0);
    data[0] = ACTION_PEN_STATUS;
    data[1] = newRole >> 8;
    data[2] = newRole & 255;
    data[3] = penNum;

    if( canBeServer && server.size() > 0 )
    {
        QList<ViewServer *>::Iterator it = server.begin();

        while( it != server.end())
        {
            QByteArray *dataCopy = new QByteArray(data);

            (*it)->queueData(dataCopy);

            // Next
            ++it;
        }
    }
#endif
}


void overlay::setDefaults(void)
{
    qDebug() << "setDefaults for page" << currentpageNum << "out of" << page.size();

    page[currentpageNum].currentBackgroundColour = defaultBackgroundColour;
    annotations->fill(page[currentpageNum].currentBackgroundColour);
    highlights->fill(Qt::transparent);

    for(int penNum=0; penNum<MAX_SCREEN_PENS; penNum ++)
    {
        page[currentpageNum].currentPens[penNum].drawStyle      = TYPE_DRAW;
        page[currentpageNum].currentPens[penNum].actionType     = ACTION_TYPE_NONE;
        page[currentpageNum].currentPens[penNum].wasDrawing     = false;
        page[currentpageNum].currentPens[penNum].posPrevPresent = false;
        page[currentpageNum].currentPens[penNum].colour         = defaultColour[penNum];
        page[currentpageNum].currentPens[penNum].applyColour    = defaultColour[penNum];
        page[currentpageNum].currentPens[penNum].thickness      = 8;
        page[currentpageNum].currentPens[penNum].pos            = QPoint(0,0);
        page[currentpageNum].currentPens[penNum].isTextPen      = false;
        page[currentpageNum].currentPens[penNum].addingText     = false;
        page[currentpageNum].currentPens[penNum].textPos        = QPoint(1,1);

        page[currentpageNum].undo[penNum].active = false;
    }

    // UNDO/REDO based on 32bit word array. This is limited in size to limit refresh delay.
    page[currentpageNum].previousActions.clear();

    for(int penNum=0; penNum<MAX_SCREEN_PENS; penNum ++)
    {
        page[currentpageNum].startPenState[penNum]   = page[currentpageNum].currentPens[penNum];

        penBrush[penNum].clear();    // No brush..
    }

    page[currentpageNum].startBackgroundColour = page[currentpageNum].currentBackgroundColour;

    // Setups for full screen text overlay (currently all at the same size)
    page[currentpageNum].textLayerFont = defaultFont;

    QFontMetrics metrics(*(page[currentpageNum].textLayerFont));
    currentFontWidth  = metrics.width("w");     // Should be a monospaced font, so okay.
    currentFontHeight = metrics.height();
    fontHeightScaling = 1.0;

    qDebug() << "defaults have been set...";
}


// TODO: replace with a set of calls to a common decoder, shared with applyEncodedAction()
// Used to either apply action to "first" stored state or to update screen image after undo
int overlay::applyActionToImage(QImage *img, QImage *highLightImg, pens_state_t *pens,
                                QColor &backColour, QVector<quint32> &actions, int next)
{
    QColor col;
    bool   penPresent[MAX_SCREEN_PENS];
    int    xLoc[MAX_SCREEN_PENS], yLoc[MAX_SCREEN_PENS];

    quint32 dat  = actions[next];
    quint32 parameter;
    qint16  signedShortParam;
    int     penNum = dat & 255;

    if( penNum < 0 || penNum > MAX_SCREEN_PENS )
    {
        // We still need to plough on...
        qDebug()  << "applyActionToImage: bad penNum (" << penNum << ")" <<
                     " startIndex=" << next << " num Stored Actions=" <<
                     actions.size();
        penNum = MAX_SCREEN_PENS - 1;
    }

    int  act     = (ACTION_UNDELETE(dat)>>24) & 255;
    bool deleted =  ACTION_IS_DELETED(dat);

    switch( act )
    {
    case ACTION_BACKGROUND_COLOUR:
        qDebug() << next << " ACTION_BACKGROUND_COLOUR" << actions[next+1];
        next++;
        if( ! deleted )
        {
            dat  = actions[next];
            col = QColor((dat>>24)&255, (dat>>16)&255, (dat>>8)&255, dat&255);

            backColour = col;

            for(penNum=0; penNum<MAX_SCREEN_PENS; penNum++)
            {
                switch(pens[penNum].drawStyle )
                {
                case TYPE_ERASER:      pens[penNum].applyColour = backColour;
                                       break;
                case TYPE_HIGHLIGHTER:
                case TYPE_DRAW:
                    break;
                }
            }
        }

        break;

    case ACTION_CLEAR_SCREEN:
        qDebug() << next << " ACTION_CLEAR_SCREEN";
        if( ! deleted )
        {
            img->fill(page[currentpageNum].currentBackgroundColour);
            highLightImg->fill(Qt::transparent);
        }

        break;

    case ACTION_SET_TEXT_CURSOR_POS:
        qDebug() << "Set text pos";
        // Not deletable as doesn't change screen view
        next ++;
        setTextPosFromPixelPos(penNum, (actions[next] >> 16 ) & 65535, (actions[next]) & 65535);
        break;

    case ACTION_APPLY_TEXT_CHAR:
        next ++;
        qDebug() << "Set text pos";
        if( ! deleted )
        {
            addTextCharacterToOverlay(penNum, actions[next]);
        }
        break;

    case ACTION_PENTYPE:
        qDebug() << next << " ACTION_PENTYPE";
        if( ! deleted )
        {
            switch( (dat>>8) & 255 )
            {
            case TYPE_DRAW:        pens[penNum].applyColour = pens[penNum].colour;
                                   pens[penNum].isTextPen   = false;
                                   break;
            case TYPE_HIGHLIGHTER: pens[penNum].applyColour = pens[penNum].colour;
                                   pens[penNum].isTextPen   = false;
                                   break;
            case TYPE_ERASER:      pens[penNum].applyColour = backColour;
                                   pens[penNum].isTextPen   = false;
                                   break;
            }
        }
        break;

    case ACTION_PENCOLOUR:
        qDebug() << next << " ACTION_PENCOLOUR";
        next ++;
        if( deleted ) qDebug() << "Bad: found deleted PENCOLOUR @ " << next << "!";

        dat  = actions[next];

        col = QColor((dat>>24)&255, (dat>>16)&255, (dat>>8)&255, dat&255);

        pens[penNum].colour = col;

        switch( pens[penNum].drawStyle )
        {
        case TYPE_DRAW:        
        case TYPE_HIGHLIGHTER: pens[penNum].applyColour = col;
                               break;
        case TYPE_ERASER:      pens[penNum].applyColour = backColour;
                               break;
        }

        break;

    case ACTION_STARTDRAWING:
        qDebug() << next << " ACTION_STARTDRAWING";
        qDebug() << " ";
        if( ! deleted )
        {
            pens[penNum].wasDrawing = false;        // TODO: This could result in first point being lost. Check why it's here.
            pens[penNum].actionType = ACTION_TYPE_DRAW;
        }

        break;

    case ACTION_STOPDRAWING:
        qDebug() << next << " ACTION_STOPDRAWING";
        if( deleted ) std::cout << "Bad: found deleted STOPDRAWING @ " << next << "!";

        pens[penNum].wasDrawing = false;
        pens[penNum].actionType = ACTION_TYPE_NONE;

        break;

    case ACTION_PENPOSITIONS:

        // Should not get here if deleted
        if( deleted ) qDebug() << "Bad: found deleted PENPOSITIONS @ " << next << "!";

        for( penNum=0; penNum<MAX_SCREEN_PENS; penNum++ )
        {
            if( (dat & (1 << penNum)) )
            {
                next ++;
                parameter          = actions[next];
                xLoc[penNum]       = ((parameter) >> 16 ) & 65535;
                yLoc[penNum]       =   parameter          & 65535;
                penPresent[penNum] = true;
            }
            else
            {
                penPresent[penNum] = false;
            }
        }
        applyPenDrawActions(img, highLightImg, pens, penPresent, xLoc, yLoc);

        break;

    case ACTION_BRUSHFROMLAST:
        qDebug() << next << " ACTION_BRUSHFROMLAST";
        if( ! deleted )
        {
            doBrushFromLast(penNum,next);
        }
        break;

    case ACTION_TRIANGLE_BRUSH:
        qDebug() << next << " ACTION_TRIANGLE_BRUSH";
        next ++;
        if( ! deleted )
        {
            explicitBrush(penNum,SHAPE_TRIANGLE,(actions[next] >> 16 ) & 65535,
                                                 actions[next] & 65535 );
        }
        break;

    case ACTION_BOX_BRUSH:
        qDebug() << next << " ACTION_BOX_BRUSH";
        next ++;
        if( ! deleted )
        {
            explicitBrush(penNum,SHAPE_BOX,(actions[next] >> 16 ) & 65535,
                                            actions[next] & 65535 );
        }
        break;

    case ACTION_CIRCLE_BRUSH:
        qDebug() << next << " ACTION_CIRCLE_BRUSH";
        next ++;
        if( ! deleted )
        {
            explicitBrush(penNum,SHAPE_CIRCLE,(actions[next] >> 16 ) & 65535,
                                               actions[next] & 65535 );
        }
        break;

    case ACTION_BRUSH_ZOOM:

        qDebug() << next << " ACTION_BRUSH_ZOOM";
        if( ! deleted )
        {
            signedShortParam = (quint16)(dat >> 8) & 65535;
            zoomBrush(penNum, signedShortParam);
        }
        break;

    case ACTION_DOBRUSH:

        qDebug() << next << " ACTION_DOBRUSH";
        next ++;
        if( ! deleted )
        {
            parameter = actions[next];
            doCurrentBrushAction(penNum, ((parameter) >> 16 ) & 65535, parameter & 65535);
        }
        break;

    case ACTION_PENWIDTH:
        qDebug() << next << " ACTION_PENWIDTH";
        if( deleted ) qDebug() << "Bad: found deleted PENWIDTH @ " << next << "!";

        pens[penNum].thickness = (dat >> 8) & 1023;

        break;

    case ACTION_ADD_STICKY_NOTE:
    case ACTION_MOVE_STICKY_NOTE:

        next ++;

        break;

    case ACTION_DELETE_STICKY_NOTE:
    case ACTION_ZOOM_STICKY_NOTE:

        break;

    default:
        qDebug() << next << " Unkonwn action type: " << std::hex << act << std::dec << ": "
                 << ACTION_DEBUG(act) << (deleted?"Del":" ");
        break;
    }

    // And skip to next for subsequent reads
    next ++;

    return next;
}



void overlay::reRenderTextLayerImage(void)
{
    if( ! textLayerImage ) return;

    textLayerImage->fill(Qt::transparent);

    QPainter textPaint(textLayerImage);

    QColor  currentColour = Qt::black;

    if( textColour.size()>0 && textColour[0].size()>0 )
    {
        currentColour = textColour[0][0];
    }

    textPaint.setPen(currentColour);

    textPaint.setFont(*(page[currentpageNum].textLayerFont));

    for(int row=0; row<textPane.size(); row ++)
    {
        int chNum   = 0;
        int startCh = 0;

        // Check for colour changes in this line
        for(chNum=0; chNum<textPane[row].size(); chNum ++)
        {
            if( textPane[row][chNum].category() != QChar::Separator_Space &&
                textColour[row][chNum]          != currentColour         )
            {
                // Draw and characters in the old colour that are required
                if( chNum > 0 )
                {
                    textPaint.drawText(currentFontWidth*startCh,
                                       currentFontHeight*fontHeightScaling*(row+1),
                                       textPane[row].mid(startCh,chNum-startCh));
                    startCh       = chNum;
                    currentColour = textColour[row][chNum];

                    textPaint.setPen(currentColour);
                }
            }
        }
        // And draw anything that is left over
        if( (chNum-startCh) > 0 )
        {
            textPaint.drawText(currentFontWidth*startCh,
                               currentFontHeight*fontHeightScaling*(row+1),
                               textPane[row].mid(startCh,chNum-startCh));
        }
    }

    for(int penNum=0; penNum<MAX_SCREEN_PENS; penNum++)
    {
        if( page[currentpageNum].currentPens[penNum].posPrevPresent )
        {
            int x = page[currentpageNum].currentPens[penNum].textPos.x() * currentFontWidth;
            int y = page[currentpageNum].currentPens[penNum].textPos.y() * currentFontHeight * fontHeightScaling;

            y = y + currentFontHeight * fontHeightScaling * 9 / 8;

            // Basically draw a ^ underneath
            textPaint.drawLine(QPoint(x,y),QPoint(x-currentFontWidth/4,y+currentFontHeight/2));
            textPaint.drawLine(QPoint(x,y),QPoint(x+currentFontWidth/4,y+currentFontHeight/2));
        }
    }
}


QString overlay::decodeWords(QVector<quint32> &word, int index)
{
    QString buf;

    switch(ACTION_TYPE(word[index]))
    {
    case ACTION_TIMEPOINT:
        return QString("TIMEPOINT %1").arg(word[index]&0xFFFFFF);

    case ACTION_BACKGROUND_PNG:
        return QString("BACKGROUND PNG IMAGE (%1 bytes)").arg(word[index]&0xFFFFFF);

    case ACTION_BACKGROUND_JPG:
        return QString("BACKGROUND JPEG IMAGE (%1 bytes)").arg(word[index]&0xFFFFFF);

    case ACTION_BACKGROUND_COLOUR:
        buf = QString("[%1,%2,%3:%4").arg((word[index+1]>>24)&255)
                                     .arg((word[index+1]>>16)&255)
                                     .arg((word[index+1]>>8)&255)
                                     .arg(word[index+1]&255);
        return QString("BACKGROUND COLOUR: ")+buf;

    case ACTION_CLEAR_SCREEN:
        return QString("ACTION_CLEAR_SCREEN");

    case ACTION_STARTDRAWING:
        return QString("PEN: %1 START DRAWING").arg(word[index]&255);

    case ACTION_STOPDRAWING:
        return QString("PEN: %1 STOP DRAWING").arg(word[index]&255);

    case ACTION_PENPOSITIONS:

        buf="PEN POSNS: ";

        for(int p=1; p<=MAX_SCREEN_PENS; p++ )
        {
            if( (word[index] & (1 << p)) )
            {
                buf += QString("%1 (%2,%3) ").arg(p-1)
                                             .arg(((word[index+p]) >> 16 ) & 65535)
                                             .arg(word[index+p] & 65535);
            }
        }
        return buf;

    case ACTION_PENCOLOUR:
        buf = QString("[%1,%2,%3:%4").arg((word[index+1]>>24)&255)
                                     .arg((word[index+1]>>16)&255)
                                     .arg((word[index+1]>>8)&255)
                                     .arg(word[index+1]&255);
        return QString("PEN: %1 COLOUR: ").arg(word[index]&255)+buf;

    case ACTION_PENWIDTH:
        return QString("PEN: %1 WIDTH %2").arg(word[index]&255).arg((word[index]>>8)&65535);

    case ACTION_PENTYPE:
        buf = QString("PEN: %1 TYPE ").arg(word[index]&255);
        switch((word[0]>>8)&255)
        {
        case TYPE_DRAW:        return buf+"DRAW";
        case TYPE_HIGHLIGHTER: return buf+"HIGHLIGHTER";
        case TYPE_ERASER:      return buf+"ERASER";
        default:               return buf+QString("Invalid(%1)").arg((word[index]>>8)&255);
        }

    case ACTION_UNDO:
        return QString("PEN: %1: UNDO").arg(word[index]&255);

    case ACTION_REDO:
        return QString("PEN: %1: REDO").arg(word[index]&255);

    case ACTION_DOBRUSH:
        return QString("PEN: %1: DO BRUSH").arg(word[index]&255);

    case ACTION_BRUSHFROMLAST:
        return QString("PEN: %1: BRUSH FROM LAST").arg(word[index]&255);

    case ACTION_TRIANGLE_BRUSH:
        return QString("PEN: %1: SET TRIANGLE BRUSH").arg(word[index]&255);

    case ACTION_BOX_BRUSH:
        return QString("PEN: %1: SET BOX BRUSH").arg(word[index]&255);

    case ACTION_CIRCLE_BRUSH:
        return QString("PEN: %1: SET CIRCLE BRUSH").arg(word[index]&255);

    case ACTION_BRUSH_ZOOM:
        return QString("PEN: %1: ZOOM BRUSH").arg(word[index]&255);

    case ACTION_ADD_STICKY_NOTE:
        return QString("PEN %1 add sticky note").arg(word[index]&255);

    case ACTION_MOVE_STICKY_NOTE:
        return QString("PEN %1 move sticky note").arg(word[index]&255);

    case ACTION_DELETE_STICKY_NOTE:
        return QString("PEN %1 delete sticky note").arg(word[index]&255);

    case ACTION_ZOOM_STICKY_NOTE:
        return QString("PEN %1 zoom sticky note").arg(word[index]&255);

    case ACTION_SET_TEXT_CURSOR_POS:
        return QString("Pen %1 Set text cursor pos from click").arg(word[index]&255);

    case ACTION_APPLY_TEXT_CHAR:
        return QString("Pen %1 add text character").arg(word[index]&255);

    default:
        return QString("Bad tag: %1").arg(word[index],1,16);
    }
}


void overlay::storeAndSendPreviousActionInt(quint32 data)
{
    page[currentpageNum].previousActions.push_back(data);
#ifndef TABLET_MODE
#ifdef SEND_ACTIONS_TO_CLIENTS
    page[currentpageNum].allActions.push_back(data);

    if( canBeServer && server.size() > 0 )
    {
        QList<ViewServer *>::Iterator it = server.begin();

        while( it != server.end())
        {
            (*it)->queue32bitData(data);

            // Next
            ++it;
        }
    }
#endif
#endif
}

void overlay::storeAllActionIntLk(quint32 sendData)
{
#ifndef TABLET_MODE
#ifdef SEND_ACTIONS_TO_CLIENTS
    page[currentpageNum].allActions.push_back(data);
#endif
    QByteArray data((const char *)&sendData, sizeof(sendData));

    if( canBeServer && server.size() > 0 )
    {
        QList<ViewServer *>::Iterator it = server.begin();

        while( it != server.end())
        {
            QByteArray *dataCopy = new QByteArray(data);

            (*it)->queueData(dataCopy);

            // Next
            ++it;
        }
    }
#endif
}

// NB This will change if we create a task to constantly grab, diff and MPEG
//    encode the full screen behind everything else.
void overlay::sendDesktopToClientsLk(void)
{
#ifndef TABLET_MODE
    // Let's not do a screengrab if there are no clients.
    if( ! canBeServer || server.size() < 1 ) return;

//    parentWindow->hide();

    // Grab a background image
    QPixmap background = screenGrabTask->currentBackground();
#if 0
    // JPEG compression needs to be multiples of 8x8
    if( (background.width() & 7) || (background.height() & 7) )
    {
        QPixmap croppedImage = background.copy(0, 0,
                                               (background.width()  & ~7),
                                               (background.height() & ~7));
        background = croppedImage;
    }
#endif
//    parentWindow->show();

    QByteArray bytes;
    QBuffer    buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);

    background.save(&buffer, "JPG",4);

    qDebug() << "Send ACTION_BACKGROUND_JPG of size:" << bytes.size() << "to all clients";

    int imageSize = bytes.size();

    if( imageSize < 134 )
    {
        qDebug() << "Bad background image size:" << bytes.size() << "Not sent.";
        return;
    }

    bytes.insert(0,"1234",4);
    bytes.data()[0] = ACTION_BACKGROUND_JPG;
    bytes.data()[1] = ( imageSize >> 16 ) & 255;
    bytes.data()[2] = ( imageSize >>  8 ) & 255;
    bytes.data()[3] = ( imageSize       ) & 255;

    switch( (bytes.size()) & 3 )
    {
    case 1:  bytes.append((char)0);
    case 2:  bytes.append((char)0);
    case 3:  bytes.append((char)0);
    }

    // and send it to any connected clients (locking the write queues)
    QList<ViewServer *>::Iterator it = server.begin();

    while( it != server.end())
    {
        QByteArray *dataCopy = new QByteArray(bytes);

        // Send the data to this thread (to a queue)
        (*it)->queueData(dataCopy);

        // Next
        ++it;
    }
#endif
}


// Clients don't retain all actions for all pages, just the current one.
// When a page change occurs, the host sends all the commands for the new page.

void overlay::sendPageChangeLk(void)
{
#ifndef TABLET_MODE
    qDebug() << "Send page change. canBeServer =" << canBeServer
             << "# clients = "                    << server.size();

    // Send
    QByteArray cmd(4,0);
    cmd.data()[0] = ACTION_CLIENT_PAGE_CHANGE;

    if( canBeServer && server.size() > 0 )
    {
        QList<ViewServer *>::Iterator it = server.begin();

        while( it != server.end())
        {
            QByteArray *cmdCopy = new QByteArray(cmd);

            // Send the data to this thread (to a queue)
            (*it)->queueData(cmdCopy);

            // Send all actions for this new page...
            sendCurrentPageState(*it,currentpageNum);

            // Next
            ++it;
        }
    }

    // Send the background to any clients
    // if( page[currentpageNum].currentBackgroundColour.alpha() < 255 )
    {
        sendDesktopToClientsLk();
    }
#endif
}


void overlay::addStickyNote(int size, QPoint pos, int penNum)
{
    qint32 header = ACTION_ADD_STICKY_NOTE << 24 | ((size & 32767)<<8) | (penNum & 255);
    qint32 position  = ((pos.x() & 65535) << 16) | (pos.y() & 65535);

    qDebug() << "Add sticky";

    // UNDO buffer: just store the colour
    storeAndSendPreviousActionInt( header );
    storeAndSendPreviousActionInt( position );
}


void overlay::deleteStickyNote(int noteNum, int penNum)
{
    qint32 header = ACTION_DELETE_STICKY_NOTE << 24 | ((noteNum & 32767)<<8) | (penNum & 255);

    qDebug() << "Delete sticky" << noteNum;

    // UNDO buffer: just store the colour
    storeAndSendPreviousActionInt( header );
}


void overlay::moveStickyNote(QPoint newPos, int stickyNum, int penNum)
{
    qint32 header   = ACTION_MOVE_STICKY_NOTE << 24 | ((stickyNum & 32767)<<8) | (penNum & 255);
    qint32 position = ((newPos.x() & 65535) << 16) | (newPos.y() & 65535);

    qDebug() << "Move sticky" << stickyNum;

    // UNDO buffer: just store the colour
    storeAndSendPreviousActionInt( header );
    storeAndSendPreviousActionInt( position );
}


void overlay::zoomStickyNote(zoom_t zoom, int stickyNum, int penNum)
{
    qint32 header = ACTION_ZOOM_STICKY_NOTE << 24 | ((stickyNum & 32767)<<8) | (penNum & 255);

    if( zoom == ZOOM_IN ) header |= 0x00800000;

    qDebug() << "Zoom sticky" << stickyNum;

    // UNDO buffer: just store the colour
    storeAndSendPreviousActionInt( header );
}


void overlay::saveBackground(void)
{
    qint32 colHeader = ACTION_BACKGROUND_COLOUR << 24;
    qint32 colour    = page[currentpageNum].currentBackgroundColour.red()   << 24 |
                       page[currentpageNum].currentBackgroundColour.green() << 16 |
                       page[currentpageNum].currentBackgroundColour.blue()  <<  8 |
                       page[currentpageNum].currentBackgroundColour.alpha();

    qDebug() << "Save background:"       << QString("%1").arg(colHeader,0,16)
             << "colour"                 << QString("%1").arg(colour,0,16)
             << "page"                   << currentpageNum
             << "Background colour is"   << page[currentpageNum].currentBackgroundColour.red()
                                         << page[currentpageNum].currentBackgroundColour.green()
                                         << page[currentpageNum].currentBackgroundColour.blue()
                                         << page[currentpageNum].currentBackgroundColour.alpha();

    // UNDO buffer: just store the colour
    //storeAndSendPreviousActionInt( colHeader );
    //storeAndSendPreviousActionInt( colour );

    // Send the background to any clients TODO: who calls this?
    //if( page[currentpageNum].currentBackgroundColour.alpha() < 255 )
    {
        sendDesktopToClientsLk();
    }
}


void overlay::setBackground(QColor bgColour)
{
    qDebug() << "setBackground() to" << bgColour << "page" << currentpageNum;
    page[currentpageNum].currentBackgroundColour = bgColour;
    backgroundColour = bgColour;

    for(int p=0; p<MAX_SCREEN_PENS; p++)
    {
        if( page[currentpageNum].currentPens[p].drawStyle == TYPE_ERASER )
        {
            page[currentpageNum].currentPens[p].applyColour = bgColour;
        }
    }

    // Store the background colour change in the rebuild list for this page
    qint32 colHeader = ACTION_BACKGROUND_COLOUR << 24;
    qint32 colour    = page[currentpageNum].currentBackgroundColour.red()   << 24 |
                       page[currentpageNum].currentBackgroundColour.green() << 16 |
                       page[currentpageNum].currentBackgroundColour.blue()  <<  8 |
                       page[currentpageNum].currentBackgroundColour.alpha();

    page[currentpageNum].previousActions.push_back(colHeader);
    page[currentpageNum].previousActions.push_back(colour);

    // And redraw the current window
//    annotations->fill(page[currentpageNum].currentBackgroundColour);
    rebuildAnnotationsFromUndoList();

    // Send the background to any clients
    saveBackground();
    qDebug() << "setBackground() currentBackground on page" << currentpageNum
             << "is" << page[currentpageNum].currentBackgroundColour;
}



void overlay::nextPage(void)
{
    currentpageNum ++;

    if( currentpageNum >= page.size() )    // New page (NB Bomproofed at end of this block)
    {
        qDebug() << "overlay::nextPage() - create a new page.";

        // New page
        pageData_t next;

        // Just fill, don't need any redraw or any such (start background colour as same as last page)
        annotations->fill(backgroundColour);
        next.startBackgroundColour   = backgroundColour;
        next.currentBackgroundColour = backgroundColour;

        // And initialise the new page structure
        for(int penNum=0; penNum<MAX_SCREEN_PENS; penNum ++)
        {
            next.startPenState[penNum].thickness      = page[currentpageNum-1].currentPens[penNum].thickness;
            next.startPenState[penNum].drawStyle      = page[currentpageNum-1].currentPens[penNum].drawStyle;
            next.startPenState[penNum].colour         = page[currentpageNum-1].currentPens[penNum].colour;
            next.startPenState[penNum].applyColour    = page[currentpageNum-1].currentPens[penNum].applyColour;
            next.startPenState[penNum].actionType     = page[currentpageNum-1].currentPens[penNum].actionType;
            next.startPenState[penNum].wasDrawing     = false;
            next.startPenState[penNum].posPrevPresent = false;
            next.startPenState[penNum].addingText     = false;
            next.startPenState[penNum].isTextPen      = false;

            next.currentPens[penNum].thickness      = next.startPenState[penNum].thickness;
            next.currentPens[penNum].drawStyle      = next.startPenState[penNum].drawStyle;
            next.currentPens[penNum].colour         = next.startPenState[penNum].colour;
            next.currentPens[penNum].applyColour    = next.startPenState[penNum].applyColour;
            next.currentPens[penNum].actionType     = next.startPenState[penNum].actionType;
            next.currentPens[penNum].wasDrawing     = next.startPenState[penNum].wasDrawing;
            next.currentPens[penNum].posPrevPresent = next.startPenState[penNum].posPrevPresent;
            next.currentPens[penNum].addingText     = next.startPenState[penNum].addingText;
            next.currentPens[penNum].isTextPen      = next.startPenState[penNum].isTextPen;

            next.currentPens[penNum].textPos        = QPoint(1,1);

            next.undo[penNum].active         = page[currentpageNum-1].undo[penNum].active;
        }
        next.textLayerFont = page[currentpageNum-1].textLayerFont;

        page.push_back(next);

        currentpageNum = page.size() - 1;  // Do not go weird on us...

        QFontMetrics metrics(*(page[currentpageNum].textLayerFont));
        currentFontWidth  = metrics.width("w");     // Should be a monospaced font, so okay.
        currentFontHeight = metrics.height();
        fontHeightScaling = 1.5;

        for(int row=0; row<textPane.size(); row++)
        {
            textPane[row] = QString();
        }
    }
    else
    {
        qDebug() << "overlay::nextPage() - advance to next pre-existing page.";

        // Next: an already drawn on page, so just redraw it
        backgroundColour = page[currentpageNum].currentBackgroundColour;
        annotations->fill(page[currentpageNum].currentBackgroundColour);

        QFontMetrics metrics(*(page[currentpageNum].textLayerFont));
        currentFontWidth  = metrics.width("w");     // Should be a monospaced font, so okay.
        currentFontHeight = metrics.height();
        fontHeightScaling = 1.5;

        for(int row=0; row<textPane.size(); row++)
        {
            textPane[row] = QString();
        }

        rebuildAnnotationsFromUndoList();

        qDebug() << "Next page: set background colour to" << backgroundColour;
    }

    // Send new page from it's start
    sendPageChangeLk();
}


void overlay::previousPage(void)
{
    if( currentpageNum<1 ) return;      // Neh, neh. Can't go back if on first page

    currentpageNum --;

    backgroundColour = page[currentpageNum].currentBackgroundColour;
    annotations->fill(page[currentpageNum].currentBackgroundColour);

    QFontMetrics metrics(*(page[currentpageNum].textLayerFont));
    currentFontWidth  = metrics.width("w");     // Should be a monospaced font, so okay.
    currentFontHeight = metrics.height();
    fontHeightScaling = 1.5;

    for(int row=0; row<textPane.size(); row++)
    {
        textPane[row] = QString();
    }

    rebuildAnnotationsFromUndoList();

    sendPageChangeLk();
}


void overlay::gotoPage(int number)
{
    if( number<0 || number>=((int)(page.size())) ) return;      // Neh, neh.

    currentpageNum = number;

    backgroundColour = page[currentpageNum].currentBackgroundColour;
    annotations->fill(page[currentpageNum].currentBackgroundColour);

    QFontMetrics metrics(*(page[currentpageNum].textLayerFont));
    currentFontWidth  = metrics.width("w");     // Should be a monospaced font, so okay.
    currentFontHeight = metrics.height();
    fontHeightScaling = 1.5;

    for(int row=0; row<textPane.size(); row++)
    {
        textPane[row] = QString();
    }

    rebuildAnnotationsFromUndoList();

    sendPageChangeLk();
}


void overlay::clearOverlay(void)
{
    quint32  cmd = ACTION_CLEAR_SCREEN << 24;
    storeAndSendPreviousActionInt(cmd);

    annotations->fill(page[currentpageNum].currentBackgroundColour);
    highlights->fill(Qt::transparent);

    for(int row=0; row<textPane.size(); row++)
    {
        textPane[row] = QString();
    }
    reRenderTextLayerImage();

    parentWidget()->update();
}



void overlay::setPenColour(int penNum, QColor col)
{
    qDebug() << "setPenColour(pen" << penNum << "col=" << col.name() << ") on page" << currentpageNum;
    qDebug() << "Page: address =" << &page;

    if( currentpageNum < 0 || currentpageNum >= page.size() )
    {
        qDebug() << "Bad currentpageNum" << currentpageNum;
        return;
    }

    if( penNum>=0 && penNum<MAX_SCREEN_PENS )
    {
        page[currentpageNum].currentPens[penNum].colour = col;

        switch( page[currentpageNum].currentPens[penNum].drawStyle )
        {
        case TYPE_DRAW:        page[currentpageNum].currentPens[penNum].applyColour = col;
                               break;
        case TYPE_HIGHLIGHTER: page[currentpageNum].currentPens[penNum].applyColour = col;
                               page[currentpageNum].currentPens[penNum].applyColour.setAlpha(33);
                               break;
        case TYPE_ERASER:      page[currentpageNum].currentPens[penNum].applyColour = page[currentpageNum].currentBackgroundColour;
                               break;
        }

        qint32 header = ACTION_PENCOLOUR << 24 | penNum;
        qint32 data   = col.red() << 24 | col.green() << 16 | col.blue() << 8 | col.alpha();

        storeAndSendPreviousActionInt( header );
        storeAndSendPreviousActionInt( data );

        qDebug() << "Pen colour change complete.";
    }
}



QColor overlay::getPenColour(int penNum)
{
    if( penNum>=0 && penNum<MAX_SCREEN_PENS )
    {
        return page[currentpageNum].currentPens[penNum].colour;
    }
    else
    {
        qDebug() << "Internal error: requested colour of bad overlay pen:" << penNum;

        return QColor(0,0,0);
    }
}




void overlay::setPenThickness(int penNum, int thick)
{
    qDebug() << "setPenThickness() for pen" << penNum << "to" << thick;

    if( penNum>=0 && penNum<MAX_SCREEN_PENS )
    {
        page[currentpageNum].currentPens[penNum].thickness = thick;

        qint32 data = ACTION_PENWIDTH << 24 | thick << 8 | penNum;

        storeAndSendPreviousActionInt( data );
    }
}



int overlay::getPenThickness(int penNum)
{
    qDebug() << "getPenThickness() for pen" << penNum;

    if( penNum>=0 && penNum<MAX_SCREEN_PENS )
    {
        return page[currentpageNum].currentPens[penNum].thickness;
    }

    return 2;   // Nasty defensive code ;)
}



void overlay::penTypeDraw(int penNum)
{
    qDebug() << "penTypeDraw(" << penNum << ")";
    if( penNum>=0 && penNum<MAX_SCREEN_PENS )
    {
//        page[currentpageNum].currentPens[penNum].actionType  = ACTION_DRAW;
        page[currentpageNum].currentPens[penNum].drawStyle   = TYPE_DRAW;
        page[currentpageNum].currentPens[penNum].applyColour = page[currentpageNum].currentPens[penNum].colour;

        qint32 data = ACTION_PENTYPE << 24 | TYPE_DRAW << 8 | penNum;

        storeAndSendPreviousActionInt( data );
    }
}

void overlay::penTypeText(int penNum)
{
    qDebug() << "penTypeText(" << penNum << ")";
    if( penNum>=0 && penNum<MAX_SCREEN_PENS )
    {
        qDebug() << "Commented out actionType = ACTION_TEXT... FIX the cause of this.";
        page[currentpageNum].currentPens[penNum].actionType  = ACTION_TYPE_TEXT;
        page[currentpageNum].currentPens[penNum].applyColour = page[currentpageNum].currentPens[penNum].colour;

        qint32 data = ACTION_TYPE_TEXT << 24 | penNum;

        storeAndSendPreviousActionInt( data );
    }
}

void overlay::penTypeHighlighter(int penNum)
{
    qDebug() << "penTypeHighlighter(" << penNum << ")";
    if( penNum>=0 && penNum<MAX_SCREEN_PENS )
    {
//        page[currentpageNum].currentPens[penNum].actionType  = ACTION_DRAW;
        page[currentpageNum].currentPens[penNum].drawStyle   = TYPE_HIGHLIGHTER;
        page[currentpageNum].currentPens[penNum].applyColour = page[currentpageNum].currentPens[penNum].colour;
        page[currentpageNum].currentPens[penNum].applyColour.setAlpha(33);

        qint32 data = ACTION_PENTYPE << 24 | TYPE_HIGHLIGHTER << 8 | penNum;

        storeAndSendPreviousActionInt( data );
    }
}

void overlay::penTypeEraser(int penNum)
{
    qDebug() << "penTypeEraser(" << penNum << ")";
    if( penNum>=0 && penNum<MAX_SCREEN_PENS )
    {
//        page[currentpageNum].currentPens[penNum].actionType  = ACTION_DRAW;
        page[currentpageNum].currentPens[penNum].drawStyle   = TYPE_ERASER;
        page[currentpageNum].currentPens[penNum].applyColour = page[currentpageNum].currentBackgroundColour;

        qint32 data = ACTION_PENTYPE << 24 | TYPE_ERASER << 8 | penNum;

        storeAndSendPreviousActionInt( data );
    }
}

void overlay::ensureTextAreaIsBigEnough(int row, int col)
{
    // Ensure we have enough strings
    while(textPane.size() <= row )
    {
        textPane.append(QString());
    }

    // Deal with any pre-padding
    while( textPane[row].size() <= col )
    {
        textPane[row] += ' ';
    }

    // And for the colour array
    if( (row+1) > textColour.size() )
    {
        textColour.resize(row+1);
    }

    while( (col+1) > textColour[row].size() )
    {
        textColour[row].append(Qt::black);
    }
}

void overlay::setTextPosFromPixelPos(int penNum, int x, int y)
{
    qDebug() << "setTextPosFromPixelPos(" << penNum << x << y
             << ") currentFontHeight ="   << currentFontHeight
             << "fontHeightScaling ="     << fontHeightScaling;

    // Get coordinates
    int  row = x /  currentFontWidth;
    int  col = (y - currentFontHeight/2) / (currentFontHeight * fontHeightScaling);

    if( col < 0 ) col = 0;

    ensureTextAreaIsBigEnough(row,col);

    // And store as the text position for this pen
    page[currentpageNum].currentPens[penNum].textPos = QPoint(row,col);

    reRenderTextLayerImage(); // This also renders the text cursors
}


void overlay::applyReturn(int &row, int &col)
{
    int spaceGap;
    int lastCharCol;

    // Skip back till we find a character
    while( col > 0 && textPane[row][col].category() == QChar::Separator_Space )
    {
        col --;
    }

    // Main skip back: skip back till we find a sufficient number of preceeding spaces
    spaceGap    = 0;
    lastCharCol = col;

    while( col >= 0 && spaceGap<8 )
    {
        if( textPane[row][col].category() == QChar::Separator_Space )
        {
            spaceGap ++;
        }
        else
        {
            spaceGap    = 0;
            lastCharCol = col;
        }

        col --;
    }

    col = lastCharCol;
    row ++;

    ensureTextAreaIsBigEnough(row,col);
}

void overlay::wordWrap(int &row, int &col)
{
    QString         wrapWord;
    QVector<QColor> wrapWordColour;

    // Skip back through any preceeding characters in the current word
    int  c = col-1;
    while( c >= 0 && textPane[row][c].category() != QChar::Separator_Space )
    {
        wrapWord.insert(0,textPane[row][c]);
        textPane[row][c] = QChar::Space;
        wrapWordColour.insert(0,textColour[row][c]);

        c --;
    }

    qDebug() << "wordWrap - wrapped:" << wrapWord;

    if( c > 0 )
    {
        // Do the normal return code:
        applyReturn(row,col);
    }
    else
    {
        col = 0;
    }

    // And spool out these characters to the new line(s)
    for( int letterNum=0; letterNum<wrapWord.length(); letterNum++ )
    {
        ensureTextAreaIsBigEnough(row,col);

        textPane[row][col]   = wrapWord[letterNum];
        textColour[row][col] = wrapWordColour[letterNum];

        col ++;
        if( col >= (textLayerImage->width()/currentFontWidth) )
        {
            applyReturn(row,col);
        }
    }

    if( c < 1 )
    {
        row ++;
        col = 0;
    }
}


void overlay::addTextCharacterToOverlay(int penNum, quint32 charData)
{
    int row = page[currentpageNum].currentPens[penNum].textPos.y();
    int col = page[currentpageNum].currentPens[penNum].textPos.x();

    int charNum = charData & ~(Qt::ShiftModifier | Qt::ControlModifier);

    qDebug() << "Add charData" << QString("%1").arg(charData,0,16)
             << "charNum"      << QString("%1").arg(charNum,0,16);

    ensureTextAreaIsBigEnough(row,col);

    // Currently, just do overwrite, so unless it's a control character, just set the value
    switch( charNum )
    {
    case Qt::Key_Backspace:
        col = (col > 0) ? (col - 1) : 0;
        textPane[row][col] = QChar(Qt::Key_Space);
        break;

    case Qt::Key_Delete:
        textPane[row].remove(col,1);
        break;

    case Qt::Key_Home:
        col = 0;
        break;

    case Qt::Key_End:
        col = textPane[row].size() - 1;
        break;

    case Qt::Key_Left:
        col = (col > 0) ? (col - 1) : 0;
        qDebug() << "Key_Left: new col =" << col;
        break;

    case Qt::Key_Up:
        row = (row > 0) ? (row - 1) : 0;
        qDebug() << "Key_Up: new row =" << row;
        break;

    case Qt::Key_Right:
        col ++;
        ensureTextAreaIsBigEnough(row,col);
        break;

    case Qt::Key_Down:
        row ++;
        ensureTextAreaIsBigEnough(row,col);
        break;

    case Qt::Key_Return:
        // Find best match on previous line
        applyReturn(row,col);
        break;

    default:
        if( charNum>=Qt::Key_A && charNum<=Qt::Key_Z )
        {
            if( col >= (textLayerImage->width()/currentFontWidth) )
            {
                wordWrap(row,col);
                ensureTextAreaIsBigEnough(row,col);
            }

            if( (charData & Qt::ShiftModifier) != 0 )
            {
                textPane[row][col] = QChar(charNum);
            }
            else
            {
                textPane[row][col] = QChar(charNum+0x20);
            }

            textColour[row][col] = page[currentpageNum].currentPens[penNum].colour;

            col ++;
        }
        else if( charNum >= Qt::Key_Space && charNum <= Qt::Key_ydiaeresis )
        {
            if( col >= (textLayerImage->width()/currentFontWidth) && charNum != Qt::Key_Space )
            {
                applyReturn(row,col);
            }

            textPane[row][col] = QChar(charNum);

            textColour[row][col] = page[currentpageNum].currentPens[penNum].colour;

            col ++;
        }
    }

    // And record the new cursor position
    page[currentpageNum].currentPens[penNum].textPos = QPoint(col,row);

    reRenderTextLayerImage();

    qDebug() << "New cursor pos:" << page[currentpageNum].currentPens[penNum].textPos;
}


// For global actions, any pen can undo these (for now).
// TODO: base this on pen session controller?
int overlay::findLastUndoableActionForPen(int penNum)
{
    int     lastStart = -1;                 // Value for not found
    int     srch      = 0;
    int     stopLoc;
    int     p;
    quint32 dat;
    int     act;

    if( page[currentpageNum].undo[penNum].active )
    {
        stopLoc = page[currentpageNum].undo[penNum].actionIndex.back(); // push_back each new undo
    }
    else
    {
        stopLoc = (int)page[currentpageNum].previousActions.size();
    }
qDebug()  << "findLastActionForPen:" << penNum << " stop @ " << stopLoc
          << " listLen=" << page[currentpageNum].previousActions.size();

    while( srch < stopLoc )
    {
        dat = page[currentpageNum].previousActions[srch];

        act = ACTION_UNDELETE(dat)>>24;
//        act = (dat>>24)&255;
//        std::cout << "entry: " << srch << " action=" << ACTION_DEBUG(act) << "(" << act << ")"<< std::endl;
        switch( act )
        {
        case ACTION_PENPOSITIONS:

            for( p=0; p<MAX_SCREEN_PENS; p++ )
            {
                if( (dat & (1 << p)) )
                {
                    if( srch >= page[currentpageNum].previousActions.size() )
                    {
                        qDebug()  << "findLastActionForPen() Saved undo data error: end of data in ACTION_PENPOSITIONS." <<
                                     " currentStoredPos="   << page[currentpageNum].previousActions.size() <<
                                     " searchPos=" << srch;
                        return -1;    // Internal error
                    }
                    srch ++;
                }
            }
            break;

        // NB Any pen can undo a background colour (new screen) command
        case ACTION_BACKGROUND_COLOUR:

            if( srch >= page[currentpageNum].previousActions.size() )
            {
                qDebug()  << "findLastActionForPen() Saved undo data error: end of data in ACTION_BACKGROUNDCOLOUR." <<
                             " currentStoredPos="   << page[currentpageNum].previousActions.size() <<
                             " searchPos=" << srch;
                return -1;    // Internal error
            }
            srch ++;
            if( ! ACTION_IS_DELETED(dat) && (srch != 1) )
            {
                qDebug() << "You gotta believe in the heart of the cards.";
                lastStart = srch-1;
            }
            break;

            // Actions signalling the start of an undo-able block
        case ACTION_CLEAR_SCREEN:

            if( ((dat & 255) == penNum) && (! ACTION_IS_DELETED(dat)) ) lastStart = srch;
            else qDebug() << srch << " skip CLEAR_SCREEN, isDeleted=" << ACTION_IS_DELETED(act);
            break;

            // Actions signalling the start of an undo-able block
        case ACTION_STARTDRAWING:

            if( ((dat & 255) == penNum) && (! ACTION_IS_DELETED(dat)) ) lastStart = srch;
            else qDebug() << srch << " skip STARTDRAWING, entryPen=" << (dat & 255) << " isDeleted=" << ACTION_IS_DELETED(act);
                break;

        // Actions signalling the start of an undo-able block
        case ACTION_DOBRUSH:
            if( ((dat & 255) == penNum) && (! ACTION_IS_DELETED(dat)) ) lastStart = srch;
            else qDebug() << srch << " skip ACTION_DOBRUSH, entryPen=" << (dat & 255) << " isDeleted=" <<
                             ACTION_IS_DELETED(dat);
            srch ++;    // coords for stamp down: (x,y)
            break;

        case ACTION_PENCOLOUR:    // No undo as no change to drawing
            srch ++;
            qDebug() << srch << " skip Colour: 0x" << std::hex << page[currentpageNum].previousActions[srch] << std::dec;
            break;

        case ACTION_TRIANGLE_BRUSH:
        case ACTION_BOX_BRUSH:
        case ACTION_CIRCLE_BRUSH:
            srch ++;
            qDebug() << srch << " skip explicit shape";
            break;

        case ACTION_PENTYPE:    // No undo as no change to drawing
        case ACTION_PENWIDTH:
        case ACTION_STOPDRAWING:
        case ACTION_BRUSHFROMLAST:
        case ACTION_BRUSH_ZOOM:
        case ACTION_DELETE_STICKY_NOTE:
        case ACTION_ZOOM_STICKY_NOTE:
            break;

        case ACTION_ADD_STICKY_NOTE:
        case ACTION_MOVE_STICKY_NOTE:
            srch ++;
            break;

        default:
            qDebug() << srch << " skip unhandled " << ACTION_DEBUG(act) << ", entryPen=" << (dat & 255) << " isDeleted=" << ACTION_IS_DELETED(dat);
        }

        srch ++;
    }
if( lastStart>=0 )
qDebug() << "delete action [" << lastStart << "] = 0x" << std::hex << page[currentpageNum].previousActions[lastStart] <<
            std::dec;

    return lastStart;
}


int overlay::findLastDrawActionForPen(int penNum, int stopLoc)
{
    int     lastStart = -1;                 // Value for not found
    int     srch      = 0;
    int     p;
    quint32 dat;
    int     act;

    qDebug() << "findLastDrawActionForPen:" << penNum << " stop @ " << stopLoc;

    while( srch < stopLoc )
    {
        dat = page[currentpageNum].previousActions[srch];

        act = ACTION_UNDELETE(dat)>>24;
        qDebug() << "entry: " << srch << " action=" << ACTION_DEBUG(act) << "(" << act << ")";
        switch( act )
        {
        case ACTION_PENPOSITIONS:

            for( p=0; p<MAX_SCREEN_PENS; p++ )
            {
                if( (dat & (1 << p)) )
                {
                    if( srch >= page[currentpageNum].previousActions.size() )
                    {
                        qDebug()  << "findLastDrawActionForPen() Saved undo data error: end of data in ACTION_PENPOSITIONS." <<
                                     " currentStoredPos="   << page[currentpageNum].previousActions.size() <<
                                     " searchPos=" << srch;
                        return -1;    // Internal error
                    }
                    srch ++;
                }
            }
            break;

        // NB Any pen can undo a background colour (new screen) command
        case ACTION_BACKGROUND_COLOUR:
            if( srch >= page[currentpageNum].previousActions.size() )
            {
                qDebug()  << "findLastDrawActionForPen() Saved undo data error: end of data in ACTION_BACKGROUNDCOLOUR." <<
                             " currentStoredPos="   << page[currentpageNum].previousActions.size() <<
                             " searchPos=" << srch;
                return -1;    // Internal error
            }
            srch ++;
            if( ! ACTION_IS_DELETED(dat) ) lastStart = srch;
            break;

        // Actions signalling the start of an undo-able block
        case ACTION_STARTDRAWING:
            if( ((dat & 255) == penNum) && (! ACTION_IS_DELETED(dat)) ) lastStart = srch;
            else qDebug() << srch << " skip STARTDRAWING, entryPen=" << (dat & 255) << " isDeleted=" << ACTION_IS_DELETED(act);
            break;

        // Here we skip actions that are composite as this function is to find last base action
        case ACTION_DOBRUSH:
            qDebug() << srch << " skip doBrush: 0x" << std::hex << page[currentpageNum].previousActions[srch] << std::dec;
            srch ++;    // Skip (x,y)
            break;

        case ACTION_PENCOLOUR:    // No undo as no change to drawing
            srch ++;
            qDebug() << srch << " skip Colour: 0x" << std::hex << page[currentpageNum].previousActions[srch] << std::dec;
            break;

        case ACTION_PENTYPE:    // No undo as no change to drawing
        case ACTION_PENWIDTH:
        case ACTION_STOPDRAWING:
            break;

        case ACTION_ADD_STICKY_NOTE:
        case ACTION_MOVE_STICKY_NOTE:

            srch ++; // fallthrough

        case ACTION_DELETE_STICKY_NOTE:
        case ACTION_ZOOM_STICKY_NOTE:

            break;

        default:
            qDebug() << srch << " skip unhandled " << ACTION_DEBUG(act) << ", entryPen=" << (dat & 255) << " isDeleted=" << ACTION_IS_DELETED(act);
        }

        srch ++;
    }
if( lastStart>=0 )
qDebug() << "delete action [" << lastStart << "] = 0x" << std::hex << page[currentpageNum].previousActions[lastStart] <<
            std::dec;

    return lastStart;
}


void overlay::deleteActionsForPen(int penNum, int readPt)
{
    quint32 dat;

    dat = page[currentpageNum].previousActions[readPt];

    if( ACTION_IS_DELETED(dat) )
    {
        qDebug()  << "deleteAction 0x" << std::hex << dat << std::dec << " @ " << readPt <<
                     " which is already deleted.";
        return;
    }

    // Note that this does not recognise deleted actions, so they are ignored too
    switch( (dat>>24)& 255 )
    {
    case ACTION_CLEAR_SCREEN:
    case ACTION_BACKGROUND_COLOUR:
    case ACTION_STARTDRAWING:
    case ACTION_DOBRUSH:
        qDebug()  << "delete " << ACTION_DEBUG((dat>>24)& 255) << " entry for pen " << penNum <<
                     " at " << readPt;
        page[currentpageNum].previousActions[readPt] = ACTION_DELETE(page[currentpageNum].previousActions[readPt]);
        break;

    case ACTION_PENCOLOUR:
    case ACTION_PENTYPE:
    case ACTION_PENWIDTH:

    case ACTION_PENPOSITIONS:
    case ACTION_STOPDRAWING:
        qDebug()  << "Ignore delete " << ACTION_DEBUG((dat>>24)& 255) << " entry for pen " << penNum <<
                     " at " << readPt;
        break;
    }

    return;
}


void overlay::undeleteActionsForPen(int penNum, int readPt)
{
    quint32 dat;

    if( readPt < 0 || readPt >= page[currentpageNum].previousActions.size() )
    {
        qDebug()  << "UNdeleteAction @ " << readPt << " which is out of range [0->" <<
                     (page[currentpageNum].previousActions.size()-1);
        return;
    }

    dat = page[currentpageNum].previousActions[readPt];

    if( ! ACTION_IS_DELETED(dat) )
    {
        qDebug()  << "UNdeleteAction 0x" << std::hex << dat << std::dec << " @ " << readPt <<
                     " which is not deleted.";
        return;
    }
    qDebug() << "UNdeleteAction 0x" << std::hex << dat << std::dec << " @ " << readPt;

    switch( ACTION_UNDELETE(dat)>>24 )
    {
    case ACTION_CLEAR_SCREEN:
    case ACTION_BACKGROUND_COLOUR:
    case ACTION_STARTDRAWING:
    case ACTION_DOBRUSH:
        qDebug() << "UNdelete at " << readPt;
        page[currentpageNum].previousActions[readPt] = ACTION_UNDELETE(page[currentpageNum].previousActions[readPt]);
        qDebug() << " becomes " <<ACTION_DEBUG_WD(page[currentpageNum].previousActions[readPt]);
        break;

    default:
        qDebug()  << "Ignore UNdelete " << ACTION_DEBUG((dat>>24)& 255) << " entry for pen " << penNum <<
                     " at " << readPt;
        break;
    }

    return;
}

void overlay::setPageInitialBackground(QColor initColour)
{
    qDebug() << "setPageInitialBackground: " << initColour;

    page[currentpageNum].startBackgroundColour = initColour;
}


void overlay::rebuildAnnotationsFromUndoList(void)
{
    pens_state_t     rebuildUpdatePens[MAX_SCREEN_PENS];
    QColor           rebuildUpdateBackground;

    qDebug() << "rebuildAnnotationsFromUndoList() - rebuild background, starting with" <<
                page[currentpageNum].startBackgroundColour;

    // Add the overlay. Start with the initial page background & update it with actions...
    // Rebuild the background colour (but don't actually draw it)
    rebuildUpdateBackground = page[currentpageNum].startBackgroundColour;

    // Set the pens to the condition at the back
    for(int p=0; p<MAX_SCREEN_PENS; p++)
    {
        rebuildUpdatePens[p] = page[currentpageNum].startPenState[p];
    }

    // Draw background with final background colour
    annotations->fill(page[currentpageNum].currentBackgroundColour);

    // Look up the most recent screen clear (we don't want to rebuild previous screens)

    qDebug() << "rebuildAnnotationsFromUndoList() - draw all elements";

    int drawIndex = 0;
    int endIndex  = page[currentpageNum].previousActions.size();

    while( drawIndex < endIndex )
    {
        drawIndex = applyActionToImage(annotations, highlights, rebuildUpdatePens,
                                       rebuildUpdateBackground,
                                       page[currentpageNum].previousActions, drawIndex);
    }

    // Just to keep us in sync
    if( page[currentpageNum].currentBackgroundColour != rebuildUpdateBackground )
    {
        qDebug() << "On page rebuild, background colour changed.";
        page[currentpageNum].currentBackgroundColour = rebuildUpdateBackground;

        // Dodgeynees:
        rebuildAnnotationsFromUndoList();
    }

    qDebug() << "After page rebuild, background colour is"
             << page[currentpageNum].currentBackgroundColour;
}


// NB In the following, we probably need to reset the stream after messing with the bytes
void overlay::undoPenAction(int penNum)
{
    if( penNum < 0 || penNum >= MAX_SCREEN_PENS )
    {
        qDebug() << "undoPenAction(): bad pen number: " << penNum;
        return;
    }
#if 0
    // Add an entry to allActions (for broadcast to new devices)
    quint32 data = ACTION_UNDO << 24 | penNum;
    storeAllActionIntLk(data);
#endif
    // Find the last action carried out by this pen (anything that draws)
    int last = findLastUndoableActionForPen(penNum);
    if( last < 0 ) return;                      // No change: not pressent, no more or error

    // Strip out any entries for this pen from 'last' to the current position
    deleteActionsForPen(penNum,last);

    // Set undo data state
    page[currentpageNum].undo[penNum].actionIndex.push_back(last);
    page[currentpageNum].undo[penNum].active = true;

    // Rebuild the view from the undo data
    rebuildAnnotationsFromUndoList();
}

void overlay::redoPenAction(int penNum)
{
    if( penNum < 0 || penNum >= MAX_SCREEN_PENS )
    {
        qDebug() << "undoPenAction(): bad pen number: " << penNum;
        return;
    }
#if 0
    // Add an entry to allActions (for broadcast to new devices)
    quint32 data = ACTION_REDO << 24 | penNum;
    storeAllActionIntLk(data);
#endif
    // Ensure we have something to redo...
    if( ! page[currentpageNum].undo[penNum].active ) return;

    // We don't need to do look-ups as we've stored this info
    int index = page[currentpageNum].undo[penNum].actionIndex.back();

    // Undelete
    undeleteActionsForPen(penNum, index);

    page[currentpageNum].undo[penNum].actionIndex.pop_back();

    // And check for no more undos
    if( page[currentpageNum].undo[penNum].actionIndex.size() < 1 )
        page[currentpageNum].undo[penNum].active = false;

    // Rebuild the view from the undo data
    rebuildAnnotationsFromUndoList();
}


void overlay::doCurrentBrushAction(int penNum, int x, int y)
{
    if( penNum<0 || penNum>=MAX_SCREEN_PENS ) return;

    qDebug() << "overlay::doCurrentBrushAction pen " << penNum <<
                " (" << x << "," << y <<") type:" <<
                PEN_TYPE_QSTR(page[currentpageNum].currentPens[penNum].drawStyle);

    QPoint cursor(x,y);
    QPoint next;
    QPoint lastPos = cursor + penBrush[penNum][0] - penBrushCenter[penNum];

    QPen     pen;
    QPen     highlightPen;

    QPainter annotationPaint(annotations);
    QPainter highlightPaint(highlights);

    switch( page[currentpageNum].currentPens[penNum].drawStyle )
    {
    case TYPE_DRAW:

        // Use current draw style for the draw-again (or brush)
        pen.setCapStyle(Qt::RoundCap);
        pen.setColor(page[currentpageNum].currentPens[penNum].colour);
        pen.setWidth(page[currentpageNum].currentPens[penNum].thickness);
        annotationPaint.setPen(pen);

        annotationPaint.setCompositionMode(QPainter::CompositionMode_Source);
        annotationPaint.setRenderHint(QPainter::HighQualityAntialiasing);

        qDebug() << "(" << lastPos.x() << "," << lastPos.y() << ") ";
        for(int i=1; i<penBrush[penNum].size(); i++)
        {
            next = cursor + penBrush[penNum][i] - penBrushCenter[penNum];
            annotationPaint.drawLine(lastPos, next);
            lastPos = next;
            qDebug() << "(" << lastPos.x() << "," << lastPos.y() << ") ";
        }

        annotationPaint.end();

        break;

    case TYPE_ERASER:

        // Use current draw style for the draw-again (or brush)
        pen.setCapStyle(Qt::RoundCap);
        pen.setColor(page[currentpageNum].currentPens[penNum].colour);  // Set to background colour
        pen.setWidth(page[currentpageNum].currentPens[penNum].thickness);

        annotationPaint.setPen(pen);
        annotationPaint.setCompositionMode(QPainter::CompositionMode_Source);
        annotationPaint.setRenderHint(QPainter::HighQualityAntialiasing);

        highlightPen.setCapStyle(Qt::RoundCap);
        highlightPen.setColor(Qt::transparent);  // Set to background colour
        highlightPen.setWidth(page[currentpageNum].currentPens[penNum].thickness);
        highlightPaint.setPen(highlightPen);

        highlightPaint.setCompositionMode(QPainter::CompositionMode_Source);
        highlightPaint.setRenderHint(QPainter::HighQualityAntialiasing);

        qDebug() << "(" << lastPos.x() << "," << lastPos.y() << ") ";
        for(int i=1; i<penBrush[penNum].size(); i++)
        {
            next = cursor + penBrush[penNum][i] - penBrushCenter[penNum];
            annotationPaint.drawLine(lastPos, next);
            highlightPaint.drawLine(lastPos, next);
            lastPos = next;
            qDebug() << "(" << lastPos.x() << "," << lastPos.y() << ") ";
        }

        annotationPaint.end();
        highlightPaint.end();

        break;

    case TYPE_HIGHLIGHTER:

        // Use current draw style for the draw-again (or brush)

        highlightPen.setCapStyle(Qt::RoundCap);
        highlightPen.setColor(page[currentpageNum].currentPens[penNum].applyColour);  // Set to background colour
        highlightPen.setWidth(page[currentpageNum].currentPens[penNum].thickness);
        highlightPaint.setPen(highlightPen);

        highlightPaint.setCompositionMode(QPainter::CompositionMode_Source);
        highlightPaint.setRenderHint(QPainter::HighQualityAntialiasing);

        qDebug() << "(" << lastPos.x() << "," << lastPos.y() << ") ";
        for(int i=1; i<penBrush[penNum].size(); i++)
        {
            next = cursor + penBrush[penNum][i] - penBrushCenter[penNum];
            highlightPaint.drawLine(lastPos, next);
            lastPos = next;
            qDebug() << "(" << lastPos.x() << "," << lastPos.y() << ") ";
        }

        highlightPaint.end();

        break;
    }
}


void overlay::CurrentBrushAction(int penNum, int x, int y)
{
    if( penNum<0 || penNum>=MAX_SCREEN_PENS )
    {
        qDebug() << "Pen" << penNum << "is bad in CurrentBrushAction()";
        return;
    }

    doCurrentBrushAction(penNum,x,y);

    // And record it in the undo/redo list
    qint32 data = ACTION_DOBRUSH << 24 | penNum;
    storeAndSendPreviousActionInt( data );
    data = (x & 65535) << 16 | (y & 65535);
    storeAndSendPreviousActionInt( data );
}


// Copy out the commands from the last undo point to here
// to a brush buffer for this pen. This will be drawn
// by the doCurrentBrushAction call. Only copy draw
// actions; not colour or thickness.

bool overlay::doBrushFromLast(int penNum)
{
    return doBrushFromLast(penNum,(int)(page[currentpageNum].previousActions.size()));
}

bool overlay::doBrushFromLast(int penNum, int endIndex)
{
    if( penNum<0 || penNum>= MAX_SCREEN_PENS ) return false;

    int lastActionStart = 0;
    // Change as want last draw action, not last undoable (e.g. a paste)
    lastActionStart = findLastDrawActionForPen(penNum,endIndex);
    if( lastActionStart < 0 ) return false;  // No previous action to use as a brush
    qDebug() << "brushFromLast(): start at " << lastActionStart << " list size=" <<
                page[currentpageNum].previousActions.size();
    penBrush[penNum].clear();

    quint32 dat;
    quint32 penPt;

    // Copy coordinates for this pen until we get a draw stop or end of data
    // (Note that this will need rework if we add new drawables such as ascii text.)

    bool foundBrushEnd = false;
    int  penMask       = 1<<penNum;
    int  chkMask;
    while( ! foundBrushEnd && lastActionStart < endIndex )
    {
        dat = page[currentpageNum].previousActions[lastActionStart];

        switch( (dat>>24)& 255 )
        {
        case ACTION_BACKGROUND_COLOUR:
        case ACTION_PENCOLOUR:
        case ACTION_SCREENSIZE:
        case ACTION_ADD_STICKY_NOTE:
        case ACTION_MOVE_STICKY_NOTE:

            lastActionStart += 2;
            break;

        case ACTION_PENTYPE:
        case ACTION_PENWIDTH:
        case ACTION_STARTDRAWING:
        case ACTION_UNDO:
        case ACTION_REDO:
        case ACTION_TRIANGLE_BRUSH:
        case ACTION_BOX_BRUSH:
        case ACTION_CIRCLE_BRUSH:
        case ACTION_DELETE_STICKY_NOTE:
        case ACTION_ZOOM_STICKY_NOTE:

            lastActionStart ++;
            break;

        case ACTION_BRUSHFROMLAST:  // Don't expect these, but...
        case ACTION_BRUSH_ZOOM:
        case ACTION_DOBRUSH:
        case ACTION_CLEAR_SCREEN:
        case ACTION_NEXTPAGE:
        case ACTION_PREVPAGE:

        case ACTION_STOPDRAWING:    // This is what I wanted.

            foundBrushEnd = true;
            break;

        case ACTION_PENPOSITIONS:

            lastActionStart ++;
            dat    &= 0x7FFFFF;
            chkMask = penMask;
            while(dat)
            {
                if(chkMask & 1)
                {
                    penPt = page[currentpageNum].previousActions[lastActionStart];
                    penBrush[penNum].push_back(QPoint((penPt>>16)&65535,penPt&65535));
                }
                if( dat & 1 ) lastActionStart ++;
                dat     >>= 1;
                chkMask >>= 1;
            }

            break;

        default:
            qDebug() << "?? ";
            lastActionStart ++;
        }
    }
    qDebug() << " end @ " << lastActionStart;
    qDebug() << "Pen point list:" << penBrush[penNum];

    if( penBrush[penNum].size() <= 0 )
    {
        qDebug() << "brushFromLast: Failed to find any draw points.";
        return false;
    }

    // And get the center for the mouse loc
    int xMax = penBrush[penNum][0].x();
    int yMax = penBrush[penNum][0].y();
    int xMin = xMax;
    int yMin = yMax;

    for(int i=1; i<penBrush[penNum].size(); i++)
    {
        if( penBrush[penNum][i].x()>xMax ) xMax = penBrush[penNum][i].x();
        if( penBrush[penNum][i].x()<xMin ) xMin = penBrush[penNum][i].x();

        if( penBrush[penNum][i].y()>yMax ) yMax = penBrush[penNum][i].y();
        if( penBrush[penNum][i].y()<yMin ) yMin = penBrush[penNum][i].y();
    }

    penBrushCenter[penNum] = QPoint( (xMax+xMin)/2, (yMax+yMin)/2 );

    return true;
}

bool overlay::brushFromLast(int penNum)
{
    if( ! doBrushFromLast(penNum) ) return false;

    // And record it in the undo/redo list
    qint32 data = ACTION_BRUSHFROMLAST << 24 | penNum;
    storeAndSendPreviousActionInt( data );

    return true;
}

void overlay::explicitBrush(int penNum, shape_t shape, int param1, int param2)
{
    if( penNum<0 || penNum>= MAX_SCREEN_PENS )
    {
        qDebug() << "ExplicitBrush for bad pen num:" << penNum;
        return;
    }

    // Replace the current brush and enable it
    penBrush[penNum].clear();

    qint32 data;

    // Note that in the following, the centre doesn't matter as an average is taken afterwards
    switch( shape )
    {
    case SHAPE_TRIANGLE:
        penBrush[penNum].push_back(QPoint(param1/2,0));
        penBrush[penNum].push_back(QPoint(-param1/2,0));
        penBrush[penNum].push_back(QPoint(0,-param1));
        penBrush[penNum].push_back(QPoint(param1/2,0));

        data = ACTION_TRIANGLE_BRUSH << 24 | penNum;
        storeAndSendPreviousActionInt( data );
        break;

    case SHAPE_BOX:
        penBrush[penNum].push_back(QPoint(0,0));
        penBrush[penNum].push_back(QPoint(0,param2));
        penBrush[penNum].push_back(QPoint(param1,param2));
        penBrush[penNum].push_back(QPoint(param1,0));
        penBrush[penNum].push_back(QPoint(0,0));

        data = ACTION_BOX_BRUSH << 24 | penNum;
        storeAndSendPreviousActionInt( data );
        break;

    case SHAPE_CIRCLE:
        for(double angle=0; angle<=2*PI; angle+=PI/50.0)
        {
            penBrush[penNum].push_back(QPoint(cos(angle)*param1,sin(angle)*param1));
        }

        data = ACTION_CIRCLE_BRUSH << 24 | penNum;
        storeAndSendPreviousActionInt( data );
        break;

    case SHAPE_NONE:
        qDebug() << "SHAPE_NONE in overlay::explicitBrush()";
        return;
    }
    // The same two 0.65535 parameters are saved
    data = (param1 & 65535) << 16 | (param2 & 65535);
    storeAndSendPreviousActionInt( data );

    // And get the center for the mouse loc
    int xMax = penBrush[penNum][0].x();
    int yMax = penBrush[penNum][0].y();
    int xMin = xMax;
    int yMin = yMax;

    for(int i=1; i<penBrush[penNum].size(); i++)
    {
        if( penBrush[penNum][i].x()>xMax ) xMax = penBrush[penNum][i].x();
        if( penBrush[penNum][i].x()<xMin ) xMin = penBrush[penNum][i].x();

        if( penBrush[penNum][i].y()>yMax ) yMax = penBrush[penNum][i].y();
        if( penBrush[penNum][i].y()<yMin ) yMin = penBrush[penNum][i].y();
    }

    penBrushCenter[penNum] = QPoint( (xMax+xMin)/2, (yMax+yMin)/2 );
}


QList<QPoint> *overlay::getBrushPoints(int penNum)
{
    return &(penBrush[penNum]);
}


void overlay::zoomBrush(int penNum, int scalePercent)
{
    if( penBrush[penNum].size() < 1 ) return;
    if( scalePercent<-99 ) return;

    double scaleFactor = (scalePercent + 100.0) / 100.0;
    int    centreX = penBrushCenter[penNum].x();
    int    centreY = penBrushCenter[penNum].y();

    for(int pt=0; pt<penBrush[penNum].size(); pt++)
    {
        penBrush[penNum][pt].setX( ((penBrush[penNum][pt].x() - centreX) * scaleFactor ) + centreX );
        penBrush[penNum][pt].setY( ((penBrush[penNum][pt].y() - centreY) * scaleFactor ) + centreY );
    }

    qint32 data = ACTION_BRUSH_ZOOM << 24 | ((scalePercent&65535) << 8) | penNum;
    storeAndSendPreviousActionInt( data );
}


void overlay::setScreenSize(int newW, int newH)
{
    qDebug() << "overlay::setScreenSize resize annotations to (" << newW << "," << newH << ") from (" <<
                annotations->width() << "," << annotations->height() << ")";

    if( w == newW && h == newH ) return;

    QImage scale;
    QSize  newSize(newW, newH);

    scale        = annotations->scaled(newSize);        // Image of all the annotations on the screen
    *annotations = scale;

    scale       = highlights->scaled(newSize);
    *highlights = scale;

    w = newW;                 // Allows us to reset size after playback
    h = newH;
}


QImage *overlay::imageData(void)
{
    return annotations;
}


void overlay::startPenDraw(int penNum)
{
    qDebug() << "startPenDraw(" << penNum << ") page" << currentpageNum;
    if( penNum < 0 || penNum >= MAX_SCREEN_PENS )  return;
    if( page[currentpageNum].currentPens[penNum].actionType != ACTION_TYPE_NONE ) return;

    // If we were undoing, a new action pops us back to the top of the undo stack
    // (but doesn't redo). The redo's are lost.

    if( page[currentpageNum].undo[penNum].active )
    {
        page[currentpageNum].undo[penNum].active = false;
        page[currentpageNum].undo[penNum].actionIndex.clear();
    }

    page[currentpageNum].currentPens[penNum].wasDrawing = false;
    page[currentpageNum].currentPens[penNum].actionType = ACTION_TYPE_DRAW;

    qint32 data = ACTION_STARTDRAWING << 24 | penNum;

    storeAndSendPreviousActionInt( data );
}


void overlay::stopPenDraw(int penNum)
{
    qDebug() << "stopPenDraw(" << penNum << ") page" << currentpageNum;
    if( penNum < 0 || penNum >= MAX_SCREEN_PENS )  return;
    if( page[currentpageNum].currentPens[penNum].actionType != ACTION_TYPE_DRAW ) return;

    page[currentpageNum].currentPens[penNum].actionType = ACTION_TYPE_NONE;

    qint32 data = ACTION_STOPDRAWING << 24 | penNum;

    storeAndSendPreviousActionInt( data );
}


bool overlay::penIsPresent(int penNum)
{
    return page[currentpageNum].currentPens[penNum].posPrevPresent;
}


void overlay::penPositions(bool penPresent[], int xLoc[], int yLoc[])
{
    if( currentpageNum < 0 || currentpageNum>20 ) qDebug() << "Bad currentPageNum:" << currentpageNum;

    // Optionally record locations to file

    bool anyChanged = false;

    int penMask = 0;

    for(int i=0; i<MAX_SCREEN_PENS; i++)
    {
        if( penPresent[i] ) penMask |= 1<<i;

        if( ! anyChanged )
        {
            if( penPresent[i] != page[currentpageNum].currentPens[i].posPrevPresent   ||
                (penPresent[i] &&
                  (xLoc[i] != penPosPrevX[i] || yLoc[i] != penPosPrevY[i] ))   )
            {
                anyChanged = true;
            }
        }
        page[currentpageNum].currentPens[i].posPrevPresent = penPresent[i];
        penPosPrevX[i] = xLoc[i];
        penPosPrevY[i] = yLoc[i];
    }

    if( anyChanged )
    {
        qint32  penMaskWord = ACTION_PENPOSITIONS << 24 | penMask;
#ifdef SEND_ACTIONS_TO_CLIENTS
        lockRemoteSendersLk();
#endif
        storeAndSendPreviousActionInt( penMaskWord );

        qint32  penCoords;

        for(int i=0; i<MAX_SCREEN_PENS; i++)
        {
            if( penPresent[i] )
            {
                penCoords = (xLoc[i] & 65535) << 16 | (yLoc[i] & 65535);

                storeAndSendPreviousActionInt( penCoords );
            }
        }
#ifdef SEND_ACTIONS_TO_CLIENTS
        unlockRemoteSenders();
#endif
        applyPenDrawActions(annotations, highlights, page[currentpageNum].currentPens, penPresent, xLoc, yLoc);
    }
}


void overlay::applyPenDrawActions(QImage *annotationImage, QImage *highlightImage, pens_state_t *pens, bool penPresent[], int xLoc[], int yLoc[])
{
    QPoint newPos;

    // Do current draw action

    QPainter annotationPaint(annotationImage);
    QPainter highlightPaint(highlightImage);
    QPen     pen;
    pen.setCapStyle(Qt::RoundCap);
    annotationPaint.setRenderHint(QPainter::HighQualityAntialiasing);
    annotationPaint.setCompositionMode(QPainter::CompositionMode_Source);
    highlightPaint.setRenderHint(QPainter::HighQualityAntialiasing);
    highlightPaint.setCompositionMode(QPainter::CompositionMode_Source);

    for(int i=0; i<MAX_SCREEN_PENS; i++)
    {
        if( penPresent[i] )
        {
            newPos.setX(xLoc[i]);
            newPos.setY(yLoc[i]);

            switch( pens[i].actionType )
            {
            case ACTION_TYPE_DRAW:

                if( pens[i].wasDrawing )
                {
#if 0
                    if( pens != page[currentpageNum].currentPens)
                    {
                        qDebug()  << i << ": DRAW " << xLoc[i] << "," << yLoc[i] << " " <<
                                     std::hex << pens[i].applyColour.name().toStdString() <<
                                     " T" << pens[i].thickness << " ";
                        qDebug()  << "D";
                    }
#endif
                    pen.setWidth(pens[i].thickness);
                    switch( pens[i].drawStyle )
                    {
                    case TYPE_DRAW:
                        pen.setColor(pens[i].applyColour);
                        annotationPaint.setPen(pen);
                        annotationPaint.drawLine(pens[i].pos, newPos);
                        break;

                    case TYPE_HIGHLIGHTER:
                        pen.setColor(pens[i].applyColour);
                        highlightPaint.setPen(pen);
                        highlightPaint.drawLine(pens[i].pos, newPos);
                        break;

                    case TYPE_ERASER:
                        pen.setColor(pens[i].applyColour);
                        annotationPaint.setPen(pen);
                        annotationPaint.drawLine(pens[i].pos, newPos);
                        pen.setColor(Qt::transparent);
                        highlightPaint.setPen(pen);
                        highlightPaint.drawLine(pens[i].pos, newPos);
                        break;
                    }
                }
                else
                {
//if( pens != currentPens) qDebug() << "1";
                    pens[i].wasDrawing = true;
                }
                break;

            case ACTION_TYPE_TEXT:
                // Not a draw action
//                break;

            default:
                pens[i].wasDrawing = false;
//                if( pens[i].wasDrawing )
//                {
//    if( pens != currentPens) qDebug() <<"x";
//                }
            }
//else
//if( pens != currentPens) qDebug() << "1";

            pens[i].pos = newPos;
        }
//else if( pens != currentPens ) qDebug() << ".";
    }
    annotationPaint.end();
//    if( pens != page[currentpageNum].currentPens)
//    {
//        qDebug() << "\n";
//    }
}


