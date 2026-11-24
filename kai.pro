QT += core gui qml quick quickcontrols2 network dbus
CONFIG += c++17 release
TARGET = kai
TEMPLATE = app

LIBS += -lLayerShellQtInterface

HEADERS += \
    src/apps.h \
    src/config.h \
    src/dockmodel.h \
    src/hyprland.h \
    src/iconprovider.h \
    src/shell.h \
    src/sysinfo.h \
    src/fonts.h \
    src/theme.h \
    src/translator.h \
    src/weather.h \
    src/windowkiller.h \
    src/audio.h \
    src/badges.h

SOURCES += \
    src/main.cpp \
    src/apps.cpp \
    src/config.cpp \
    src/dockmodel.cpp \
    src/hyprland.cpp \
    src/iconprovider.cpp \
    src/shell.cpp \
    src/sysinfo.cpp \
    src/theme.cpp \
    src/translator.cpp \
    src/weather.cpp \
    src/windowkiller.cpp \
    src/audio.cpp \
    src/badges.cpp

include(shaders.pri)
RESOURCES += src/resources.qrc
