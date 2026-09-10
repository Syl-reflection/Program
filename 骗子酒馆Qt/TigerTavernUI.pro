QT += widgets svg network
CONFIG += c++17
TEMPLATE = app
TARGET = TigerTavernUI

msvc: QMAKE_CXXFLAGS += /utf-8

SOURCES += \
    main.cpp \
    QtWidgetsApplication1.cpp \
    LobbyDialog.cpp \
    NetworkClient.cpp \
    NetworkHost.cpp

HEADERS += \
    QtWidgetsApplication1.h \
    LobbyDialog.h \
    NetworkClient.h \
    NetworkHost.h \
    Protocol.h

FORMS += \
    QtWidgetsApplication1.ui

RESOURCES += \
    LiarsTavern.qrc
