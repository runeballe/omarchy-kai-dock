QT += core gui qml quick testlib network dbus
CONFIG += c++17 testcase
TARGET = kai_tests
TEMPLATE = app
INCLUDEPATH += ../src
HEADERS += ../src/apps.h ../src/config.h ../src/dockmodel.h ../src/hyprland.h ../src/sysinfo.h ../src/theme.h ../src/translator.h ../src/weather.h ../src/windowkiller.h ../src/audio.h ../src/badges.h ../src/scriptapplets.h
SOURCES += kai_tests.cpp ../src/apps.cpp ../src/config.cpp ../src/dockmodel.cpp ../src/hyprland.cpp ../src/sysinfo.cpp ../src/theme.cpp ../src/translator.cpp ../src/weather.cpp ../src/windowkiller.cpp ../src/audio.cpp ../src/badges.cpp ../src/scriptapplets.cpp
include(../shaders.pri)
RESOURCES += ../src/resources.qrc
