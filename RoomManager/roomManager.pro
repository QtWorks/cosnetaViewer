#-------------------------------------------------
#
# Project created by QtCreator 2016-01-18T09:04:28
#
#-------------------------------------------------

QT += core network gui widgets qml quick quickwidgets
android: QT += androidextras
INCLUDEPATH += $$PWD/../freeStyleQt ../cosnetaapi

DEFINES += ROOMMANAGER_LIBRARY
TEMPLATE = lib

CONFIG(debug, debug|release) {
    TARGET = roomMgrd
} else {
    TARGET = roomMgr
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
    roommanager.h \
    roommanager_global.h \
    room.h

SOURCES += \
    roommanager.cpp \
    room.cpp


