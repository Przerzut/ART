cmake_minimum_required(VERSION 3.16)
project(ART VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Network Charts Positioning Quick QuickWidgets Qml)

add_executable(ART
    main.cpp
    mainwindow.cpp
    mainwindow.h
    trammodel.cpp
    trammodel.h
    resources.qrc
)

target_link_libraries(ART PRIVATE 
    Qt5::Core 
    Qt5::Gui 
    Qt5::Widgets 
    Qt5::Network 
    Qt5::Charts
    Qt5::Positioning
    Qt5::Quick
    Qt5::QuickWidgets
    Qt5::Qml
)