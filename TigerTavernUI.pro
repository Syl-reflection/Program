QT += widgets svg
CONFIG += c++17
TEMPLATE = app
TARGET = TigerTavernUI

msvc: QMAKE_CXXFLAGS += /utf-8

SOURCES += \
    main.cpp \
    QtWidgetsApplication1.cpp

HEADERS += \
    QtWidgetsApplication1.h

FORMS += \
    QtWidgetsApplication1.ui

RESOURCES += \
    LiarsTavern.qrc
