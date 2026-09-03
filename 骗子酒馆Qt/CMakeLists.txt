cmake_minimum_required(VERSION 3.16)
project(TigerCardEventGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

add_executable(TigerTavernUI WIN32
    main.cpp
    LobbyDialog.cpp
    NetworkClient.cpp
    NetworkHost.cpp
    QtWidgetsApplication1.cpp
    LobbyDialog.h
    NetworkClient.h
    NetworkHost.h
    Protocol.h
    QtWidgetsApplication1.h
    QtWidgetsApplication1.ui
)

target_link_libraries(TigerTavernUI PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)

if(MSVC)
    target_compile_options(TigerTavernUI PRIVATE /utf-8)
endif()
