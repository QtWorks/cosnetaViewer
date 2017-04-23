######################################################################
# Automatically generated by qmake (2.01a) Sun Dec 16 19:21:55 2012
######################################################################

TEMPLATE = app
TARGET = sysTrayQt
DEPENDPATH += .
INCLUDEPATH += .

# Input
HEADERS += appComms.h \
           GUI.h \
           main.h \
           gestures.h \
           pen_location.h \
           build_options.h \
           networkReceiver.h \
           dongleReader.h \
           dongleData.h \
           net_connections.h \
           updater.h \
           licenseManager.h \
           savedSettings.h \
           radarResponder.h
#           freerunner_hardware.h \
#           freerunner_low_level.h \
#           FreerunnerReader.h \

SOURCES += appComms.cpp \
           GUI.cpp \
           main.cpp \
           gestures.cpp \
           pen_location.cpp \
           networkReceiver.cpp \
           dongleReader.cpp \
           updater.cpp \
           licenseManager.cpp \
           savedSettings.cpp \
           radarResponder.cpp
#           FreerunnerReader.cpp \
#           freerunner_low_level_linux.cpp \
#           freerunner_low_level_mac.cpp \
#           freerunner_low_level_windows.cpp \

RESOURCES += sysTrayQt.qrc
QT        += opengl
QT        += network
QT        += gui

# Icons
win32:RC_FILE += sysTrayQt.rc
macx:ICON = images/cosneta-logo.icns

win32:CONFIG(release, debug|release): LIBS += -L"C:/Program Files (x86)/Windows Kits/8.0/Lib/win8/um/x86/" -lcfgmgr32 -lsetupapi -lUser32
else:win32:CONFIG(debug, debug|release): LIBS += -L"C:/Program Files (x86)/Windows Kits/8.0/Lib/win8/um/x86/" -lcfgmgr32 -lsetupapi -lUser32

# Only required for low level USB access (to use Flip) and we don't have any for now.
# win32:QMAKE_LFLAGS += /MANIFESTUAC:"level='highestAvailable'"

win32:INCLUDEPATH += "C:/Program Files (x86)/Windows Kits/8.0/Lib/win8/um/x86"
win32:DEPENDPATH += "C:/Program Files (x86)/Windows Kits/8.0/Lib/win8/um/x86"

macx: MAC_SDK = /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk
macx: INCLUDEPATH += $$QMAKE_MAC_SDK/System/Library/Frameworks/CoreFoundation.framework/Headers
macx: DEPENDPATH += $$QMAKE_MAC_SDK/System/Library/Frameworks/CoreFoundation.framework/Headers
macx: INCLUDEPATH += $$QMAKE_MAC_SDK/System/Library/Frameworks/IOKit.framework/Headers
macx: DEPENDPATH += $$QMAKE_MAC_SDK/System/Library/Frameworks/IOKit.framework/Headers
macx:LIBS += -framework CoreFoundation -framework IOKit

# Warnings in Windows (MSVS only)
#win32: DEFINES +=-D _CRT_SECURE_NO_WARNINGS

OTHER_FILES += \
    sysTrayQt.rc \
    ReadMe.txt

## For development (both release and debug builds)
#QMAKE_CFLAGS += -O0
# Release only values
#msvc:QMAKE_CXXFLAGS_RELEASE += /O2
#gcc:QMAKE_CXXFLAGS_RELEASE += -O2

