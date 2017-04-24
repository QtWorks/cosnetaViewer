#ifndef TEXTOVERLAY_H
#define TEXTOVERLAY_H

#include <QImage>
#include <QFont>
#include <QString>
#include <QVector>
#include <QColor>

typedef struct
{
    QVector<QString>           textLine;
    QVector< QVector<QColor> > charColour;
    int              fontCharWidthPx;   // Width of text characters on this page
    int              fontCharHeightPx;  // Height of text characters on this page

} sizedTextLayer;


class textOverlay
{
public:
    textOverlay(int width, int height);
    ~textOverlay();

    void    addCharacter(QChar character, int pixelX, int pixelY, int sz, QColor colour);
    void    paintImage(QPainter *painter);
    void    resize(int width, int height);

private:
    sizedTextLayer *getTextLayer(int fontHeight);
    void            eraseRectFromAllPlanes(int &inX, int &inY, int &inWidth, int &inHeight);
    void            redrawImage(int x, int y, int width, int height);

    QImage    drawImage;
    QFont     font;
    int       lastFontHeight;

    QVector<sizedTextLayer> textLayer;
};

#endif // TEXTOVERLAY_H
