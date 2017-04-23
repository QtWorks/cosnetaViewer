#ifndef LICENSEMANAGER_H
#define LICENSEMANAGER_H

#include <QByteArray>
#include <QObject>

class licenseManager : public QObject
{
    Q_OBJECT
public:
    explicit licenseManager(QObject *parent = 0);
    
    bool     haveValidLicense(void);

signals:
    
public slots:
    
private:
    bool        createLocalFileObfuscator(void);

    QByteArray  thisExeSha;
};

#endif // LICENSEMANAGER_H
