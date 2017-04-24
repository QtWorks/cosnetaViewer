// TODO: finish use this... may not, but feels like a cleaner design

#include "overlaySerial.h"

#include <QObject>

typedef enum { ACTION_NONE=1, ACTION_DRAW } pen_action_t;

overlaySerial::overlaySerial(QObject *parent) : QObject(parent)
{
    isAFile = false;
}


explicit overlaySerial(QObject *parent = 0);
explicit overlaySerial(QString fileName, QObject *parent = 0);
~overlaySerial();

bool  overlaySerial::fileSave(QString fileName)
{
    file.setFileName(fileName);

    if( ! file.open(QIODevice::WriteOnly|QIODevice::Truncate) )
    {
        // Failed to start saving the discussion.
        return false;
    }

    fileStream.setDevice(&saveFile);
    fileStream.setVersion(QDataStream::Qt_5_0);

    isAFile = true;

    lastTimepoint[0] = 0xFF; // Ensure that the 'previous timepoint' triggers a write
    sessionTime.restart();    // Keep a time in milli-seconds

    return true;
}

bool  overlaySerial::fileRead(QString fileName)
{
    file.setFileName(fileName);

    if( ! file.open(QIODevice::ReadOnly) )
    {
        // Failed to start reading the saved discussion.
        return false;
    }

    fileStream.setDevice(&saveFile);
    fileStream.setVersion(QDataStream::Qt_5_0);

    isAFile = true;

    return true;
}


// Build data
void  overlaySerial::addPenThicknessChange(int penNum, int thickness)
{
    if( isAFile )
    {
        ;
    }
}

void  overlaySerial::addPenTypeChange(int penNum, pen_draw_style_t type);
void  overlaySerial::addPenColourChange(int penNum, QColor colour);
void  overlaySerial::addBackGroundColourChange(QColor colour);
void  overlaySerial::addBackgroundImage(QPixmap img);

// Retreive data
void           overlaySerial::setIndex(int entry);

actionId_t     overlaySerial::getActionId(void);
int            overlaySerial::getPenNum(void);
penDrawStyle_t overlaySerial::getPenType(int penNum);
int            overlaySerial::getPenWidth(int penNum);
QColor         overlaySerial::getColour(void);
void           overlaySerial::getPenPositions(int &xPos[MAX_PENS], int yPos[MAX_PENS]);


void           overlaySerial::writeTimePointToFile(void)
{
    int           tm = sessionTime.elapsed() / 10;             // 1/100 sec, not 1/1000
    unsigned char timeWord[4] = { ACTION_TIMEPOINT,
                                  ( tm >> 16) & 255, ( tm >> 8) & 255, tm & 255 };
    unsigned char top = (tm >> 24) & 255;

    if( lastTimepoint[0] != top          || lastTimepoint[1] != timeWord[1] ||
        lastTimepoint[2] != timeWord[2]  || lastTimepoint[3] != timeWord[3]   )
    {
        if( fileStream.writeRawData((const char *)timeWord,sizeof(timeWord)) < 0 )
        {
            std::cerr << saveFile.errorString().toStdString() << std::endl;
        }

        lastTimepoint[0] = top;
        lastTimepoint[1] = timeWord[1];
        lastTimepoint[2] = timeWord[2];
        lastTimepoint[3] = timeWord[3];
    }
}


