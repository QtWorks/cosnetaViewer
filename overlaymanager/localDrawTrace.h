#ifndef LOCALDRAWTRACE_H
#define LOCALDRAWTRACE_H

#include <QWidget>
#include <QImage>
#include <QVector>
#include <QPoint>
#include <QFile>
#include <QDataStream>
#include <QStandardPaths>
#include <QString>
#include <QTextDocument>
#include <QPrinter>


#define EXPIREY_UPDATES (40*2)

typedef struct
{
    QColor          colour;
    int             thickness;
    int             endCounter;
    QVector<QPoint> points;

} lineSegment_t;

typedef struct
{
    QColor           pColour;
    int              pThickness;
    int              pEndCounter;
    QVector<QPoint>  pPoints;

} pLineSegment_t;

class localDrawTrace : public QWidget
{
    Q_OBJECT
public:
    explicit localDrawTrace(QImage *image, QWidget *parent = 0);
    ~localDrawTrace();

    bool     localDrawCachePresent(void);
    void     updateLocalDrawCache(int x, int y, bool draw);
    void     resetDrawCache(void);
    bool     privateNoteDrawCachePresent(void);
    void     updatePrivateLocalDrawCache(int x, int y, bool draw);
    void     redrawPrivateDraw(void);
    void     resetPrivateDrawCache(void);
    void     fillPrivateBackground(QColor newColour);
    void     setPenColour(QColor newColour);
    void     setPenThickness(int newThickness);
    void     setPPenColour(QColor newPColour);
    void     setPPenThickness(int newPThickness);
    void     localDrawSave(void);
    void     privateNoteUndo(void);
    void     privateNoteSave(void);

    QFile    localDrawFile;
    QFile    privateNoteFile;


signals:

public slots:

private:
    QImage                  *annotation;
    int                     width, height;
    QColor                  colour;
    int                     thickness;

    int                     counter;
    bool                    drawing;
    int                     currentLine;
    int                     lastDraw;
    bool                    linesInArray;

    QVector<lineSegment_t> lines;

    QImage    *pAnnotation;
    QColor     pColour;
    int        pThickness;

    int        pCounter;
    bool       pDrawing;
    int        pCurrentLine;
    int        pLastDraw;
    bool       pLinesInArray;

    QVector<pLineSegment_t> pLines;
};

#endif // LOCALDRAWTRACE_H
