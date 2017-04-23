#ifndef BASEVIEW_H
#define BASEVIEW_H

// Qt
#include <QQuickView>

class BaseView : public QQuickView
{
public:
    // Constructor
    BaseView(QWindow *parent=NULL);

    // Set context property
    void setContextProperty(const QString &sPropertyName, QObject *pObject);
};

#endif // BASEVIEW_H
