QT += core gui qml quick testlib network
CONFIG += c++17 testcase
TARGET = kai_tests
TEMPLATE = app
INCLUDEPATH += ../src
HEADERS += ../src/apps.h ../src/config.h ../src/dockmodel.h ../src/hyprland.h ../src/sysinfo.h ../src/theme.h ../src/translator.h ../src/weather.h ../src/windowkiller.h
SOURCES += kai_tests.cpp ../src/apps.cpp ../src/config.cpp ../src/dockmodel.cpp ../src/hyprland.cpp ../src/sysinfo.cpp ../src/theme.cpp ../src/translator.cpp ../src/weather.cpp ../src/windowkiller.cpp
RESOURCES += ../src/resources.qrc
