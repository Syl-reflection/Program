QT += widgets network
CONFIG += c++17
TEMPLATE = app
TARGET = LiarsTavern

msvc: QMAKE_CXXFLAGS += /utf-8

SOURCES += \
    main.cpp \
    QtWidgetsApplication1.cpp

HEADERS += \
    QtWidgetsApplication1.h
