#ifndef PENCURSOR_H
#define PENCURSOR_H

// This object is mainly just a window with no border with a fixed
// shape with selectable colour. The position and visibility is
// inherited behaviour.


#include <QWidget>
#include <QPixmap>
#include <QColor>
#include <QVector>

typedef enum cursorType_e {CURS_NORMAL, CURS_BRUSH, CURS_STICKY, CURS_LASER,
                           CURS_DELETE, CURS_MOVE,  CURS_TEXT                } cursorType_t;
typedef enum penMode_e { MODE_OVERLAY, MODE_PRESENTATION, MODE_INACTIVE      } penMode_t;

class penCursor : public QWidget
{
    Q_OBJECT
public:
    explicit   penCursor(QColor colour = Qt::magenta, QWidget *parent = 0);
               ~penCursor(void);

    void       updatePenColour(QColor colour);
    QColor     getCurrentColour(void);

    void       updatePenSize(int newSize);
    int        getCurrentPenSize(void);

    void       setCursorFromBrush(QList<QPoint> *brush);
    void       setStickyAddBrush(int size, QColor colour);
    void       setDeleteBrush(void);
    void       setTextBrush(void);
    void       setMoveBrush(void);
    void       setdefaultCursor(void);

    void       removeStickyAddBrush(void);

    void       moveCentre(int x, int y);

    void       setInactiveMode(void);
    void       setOverlayMode(void);
    void       setPresentationMode(void);

    void       togglePresentationPointer(void);

protected:
    void       paintEvent(QPaintEvent *);

public slots:

private:
    void            rebuildImage(void);
    void            rebuildStandardCursor(void);
    void            rebuildBrushCursor(void);
    void            rebuildStickyCursor(void);
    void            rebuildDeleteCursor(void);
    void            rebuildMoveCursor(void);
    void            rebuildTextCursor(void);
    void            rebuildPresentationCursor(void);

    QImage          cursorImage;
    QString         moveImageRes;
    QString         textImageRes;
    QColor          penColour;
    QColor          filledColour;
    int             penSize;
    int             outerSize;

    cursorType_t    type;
    cursorType_t    beforeStickyType;
    QVector<QPoint> brushPoints;
    int             brushWidth;
    int             brushHeight;
    float           opaqueness;

    bool            presentationLaserIsOn;
    int             presentationBlobSize;
    int             stickyWidth, stickyHeight;
    penMode_t       penMode;
};

#endif // PENCURSOR_H
