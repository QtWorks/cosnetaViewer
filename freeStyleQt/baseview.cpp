// Qt
#include <QQmlContext>

// Application
#include "baseview.h"

// Constructor
BaseView::BaseView(QWindow *parent) : QQuickView(parent)
{
    setResizeMode(QQuickView::SizeRootObjectToView);
}

// Set context property
void BaseView::setContextProperty(const QString &sPropertyName, QObject *pObject)
{
    rootContext()->setContextProperty(sPropertyName, pObject);
}
