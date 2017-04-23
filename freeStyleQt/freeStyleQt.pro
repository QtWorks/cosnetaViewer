#-------------------------------------------------
#
# Project created by QtCreator 2016-01-18T09:04:28
#
#-------------------------------------------------

QT += core gui qml quick quickwidgets svg network widgets printsupport
android:QT += androidextras

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app
INCLUDEPATH += $$PWD/../cosnetaapi
!android:!ios:DEPENDPATH += $$PWD/../cosnetaapi

win32:CONFIG(release, debug|release):    LIBS += -L$$PWD/../bin -lcosnetaAPI
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../bin -lcosnetaAPId
else:macx:                               LIBS += -L$$PWD/../builds/cosnetaAPI/ -lcosnetaAPI
else:unix:!android:!ios:                 LIBS += -L$$PWD/../builds/cosnetaAPI-Release/ -lcosnetaAPI

CONFIG(debug, debug|release) {
    TARGET = freestyleqtd
} else {
    TARGET = freestyleqt
}

unix {
    DESTDIR = ./bin
    MOC_DIR = ./moc
    OBJECTS_DIR = ./obj
}

win32 {
    DESTDIR = .\\bin
    MOC_DIR = .\\moc
    OBJECTS_DIR = .\\obj
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
    controller.h \
    defs.h \
    freestyleqt.h \
    iservice.h \
    baseview.h \
    csingleton.h \
    servermanager.h

SOURCES += \
    controller.cpp \
    freestyleqt.cpp \
    main.cpp \
    baseview.cpp \
    servermanager.cpp

RESOURCES += \
    resources.qrc
