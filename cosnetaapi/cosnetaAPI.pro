#-------------------------------------------------
#
# Project created by QtCreator 2013-01-27T12:29:49
#
#-------------------------------------------------

QT       -= gui

TEMPLATE = lib

DEFINES += COSNETAAPI_LIBRARY

CONFIG(debug, debug|release) {
    TARGET = cosnetaAPId
} else {
    TARGET = cosnetaAPI
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

SOURCES += cosnetaAPI.cpp \
    trayComms.cpp

HEADERS +=\
    cosnetaAPI.h \
    sharedMem.h \
    trayComms.h \
    cosnetaAPI_internal.h

