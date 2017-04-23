#-------------------------------------------------
#
# Project created by QtCreator 2017-04-23T20:26:34
#
#-------------------------------------------------

QT       -= gui

TARGET = overlaymanager
TEMPLATE = lib

DEFINES += OVERLAYMANAGER_LIBRARY

SOURCES += overlaymanager.cpp

HEADERS += overlaymanager.h\
        overlaymanager_global.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
