#ifndef MENUENTRY_H
#define MENUENTRY_H


#include <QObject>
#include <QImage>
#include <QString>
#include <vector>

#include "clickType.h"

using namespace std;    // For vector

class menuEntry : public QObject
{
    Q_OBJECT

public:
    explicit menuEntry(bool eventOnEntry, int optParam = 0);   // the optional parameter is to identify the instance (eg which pen)

    bool     haveMultipleOptions(void);
    QImage  *getCurrentOptionImage();
    QImage  *getNextOptionImage(void);
    QImage  *getPrevOptionImage(void);
    int      addOption(QString text, int newValue);   // Return ID for alter option
    void     alterOption(int id, QString text, int newValue);
    void     selectOption(int valueIndex);
    void     applyAction(click_type_t action);

// QObject::connect: No such signal menuEntry::entryViewUpdateRequired(int optionalParam)
// QObject::connect: No such signal menuEntry::entryChanged(void *newValue, int optionalParam)
signals:
    void     entryViewUpdateRequired(int optionalParam);
    void     entryChanged(int newValue, int optionalParam);

public slots:

private:
    int               optionalParameter;
    int               numOptions;
    int               currentOption;
    int               selectingOption;
    vector<QImage *>  cachedImage;
    vector<QString>   name;
    vector<int>       value;
    bool              eventWhileSelecting;
    
};

#endif // MENUENTRY_H
