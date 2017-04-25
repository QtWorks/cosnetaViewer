#-------------------------------------------------
#
# Project created by QtCreator 2016-01-18T09:04:28
#
#-------------------------------------------------

QT += core network gui widgets
android: QT += androidextras
INCLUDEPATH += $$PWD/../freeStyleQt ../cosnetaapi

DEFINES += OVERLAYMANAGER_LIBRARY
TEMPLATE = lib

CONFIG(debug, debug|release) {
    TARGET = overlaymgr
} else {
    TARGET = overlaymgr
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
    overlaymanager.h \
    overlaymanager_global.h

SOURCES += \
    overlaymanager.cpp


