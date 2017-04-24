#-------------------------------------------------
#
# Project created by QtCreator 2016-01-18T09:04:28
#
#-------------------------------------------------

QT += core network gui widgets
android: QT += androidextras
INCLUDEPATH += $$PWD/../freeStyleQt ../cosnetaapi

DEFINES += SERVERMANAGER_LIBRARY
TEMPLATE = lib

CONFIG(debug, debug|release) {
    TARGET = servermgrd
} else {
    TARGET = servermgr
}

unix {
    DESTDIR = ../bin
    MOC_DIR = ../moc
    OBJECTS_DIR = ../obj
}

win32 {
    DESTDIR = ..\\bin
    MOC_DIR = ..\\moc
    OBJECTS_DIR = ..\\obj
}

unix {
    QMAKE_CLEAN *= $$DESTDIR/*$$TARGET*
    QMAKE_CLEAN *= $$MOC_DIR/*moc_*
    QMAKE_CLEAN *= $$OBJECTS_DIR/*.o*
}

win32 {
    QMAKE_CLEAN *= $$DESTDIR\\*$$TARGET*
    QMAKE_CLEAN *= $$MOC_DIR\\*moc_*
    QMAKE_CLEAN *= $$OBJECTS_DIR\\*.o*
}

HEADERS += \
    servermanager.h \
    servermanager_global.h

SOURCES += \
    servermanager.cpp
