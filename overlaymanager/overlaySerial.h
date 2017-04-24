// TODO: finish use this... may not, but feels like a cleaner design

#ifndef OVERLAYSERIAL_H
#define OVERLAYSERIAL_H

#include <QObject>
#include <QFile>
#include <QDataStream>
#include <QColor>
#include <QTime>

#include "../cosnetaAPI/cosnetaAPI.h"

typedef enum { TYPE_DRAW=1,   TYPE_HIGHLIGHTER, TYPE_ERASER } penDrawStyle_t;

typedef enum ACTION_id_e
{
    ACTION_TIMEPOINT=1,
    ACTION_BACKGROUND_PNG, ACTION_BACKGROUND_COLOUR,
    ACTION_STARTDRAWING,   ACTION_STOPDRAWING,
    ACTION_PENPOSITIONS,
    ACTION_PENCOLOUR, ACTION_PENWIDTH, ACTION_PENTYPE,

    ACTION_ERROR        // End of data or similar error

} actionId_t;


class overlaySerial : public QObject
{
    Q_OBJECT
public:
    explicit overlaySerial(QObject *parent = 0);
    explicit overlaySerial(QString fileName, int openMode, QObject *parent = 0);
    ~overlaySerial();

    bool  fileSave(QString fileName);
    bool  fileRead(QString fileName);

    // Build data
    void  addPenThicknessChange(int penNum, int thickness);
    void  addPenTypeChange(int penNum, penDrawStyle_t type);
    void  addPenColourChange(int penNum, QColor colour);
    void  addBackGroundColourChange(QColor colour);
    void  addBackgroundImage(QPixmap img);
    
    // Retreive data
    void           setIndex(int entry);

    int            getDelay(void);
    actionId_t     getActionId(void);
    int            getPenNum(void);
    penDrawStyle_t getPenType(int penNum);
    int            getPenWidth(int penNum);
    QColor         getColour(void);
    void           getPenPositions(int xPos[MAX_PENS], int yPos[MAX_PENS]);


signals:
    
public slots:

private:
    void          writeTimePointToFile(void);

    QTime         sessionTime;
    unsigned char lastTimepoint[4];

    bool          isAFile;
    bool          writeToFile;
    QFile         file;
    QDataStream   fileStream;
    
};

#endif // OVERLAYSERIAL_H
