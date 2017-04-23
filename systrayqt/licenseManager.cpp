#include "licenseManager.h"

#include <QApplication>
#include <QFile>
#include <qcryptographichash.h>
#include <QDebug>


licenseManager::licenseManager(QObject *parent) : QObject(parent)
{
    if( ! createLocalFileObfuscator() )
    {
        qDebug() << "Failed to generate the local key.";
        return;
    }

    qDebug() << "Opened and closed file for reading";
}


bool licenseManager::haveValidLicense(void)
{
    if( thisExeSha.length()<20 )
    {
        qDebug() << "No license check possible as failed local key.";
        return false;
    }

    // License is stored in a file called license.dat



    return true;
}


bool licenseManager::createLocalFileObfuscator(void)
{
    QCryptographicHash hash(QCryptographicHash::Sha3_256);
    hash.reset();

    QFile thisExe(QApplication::applicationFilePath());
    if( ! thisExe.open(QIODevice::ReadOnly) )
    {
        qDebug() << "Failed top open this executable...";
        thisExeSha.clear();
        return false;
    }

    // Add a known salt value

    char hashData[5] = "A1@h";

    hash.addData(hashData,sizeof(hashData));

    // Read the file in to generate an encrypted value
    while( thisExe.read(hashData,sizeof(hashData)) == sizeof(hashData) )
    {
        hash.addData(hashData,sizeof(hashData));
    }

    // And a trailer
    hash.addData("{} *",4);

    thisExe.close();

    thisExeSha = hash.result();

    return true;
}

