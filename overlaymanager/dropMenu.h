#ifndef DROPMENU_H
#define DROPMENU_H

#include <QWidget>
#include <QString>
#include <vector>

#include "menuEntry.h"
#include "clickType.h"

using namespace std;    // For vector
#if 0
typedef struct
{
    vector<QString> entryName;

} menuOption_t;
#endif

class dropMenu : public QWidget
{
    Q_OBJECT

public:
    explicit dropMenu(QWidget *parent = 0);

    int  addOption(menuEntry *entry);
    void addEntry(int optionNum, QString name);                // Add a signal when an external event is triggered (rather than a menu update).
    bool pointerAndclickMenu(int xLoc, int yLoc, click_type_t type); // Pointer clicked. Do any updates.
    void setColour(QColor newColour);
    void cacheImage(void);                                     // render to an untername image for screen blitting (faster start up)
    void setScreenSize(int w, int h);
    
public slots:

protected:
    void paintEvent(QPaintEvent *event);

private:
    vector<menuEntry *> option;
    QImage             *menuImage;
    QColor              colour;
    int                 currentWidth;
    int                 currentHeight;
    int                 scnWidth;
    int                 scnHeight;

};

#endif // DROPMENU_H
