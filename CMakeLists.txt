cmake_minimum_required(VERSION 3.16)
project(ART VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets Network Charts REQUIRED)

add_executable(ART
    main.cpp
    mainwindow.cpp
    mainwindow.h
    resources.qrc
)

target_link_libraries(ART PRIVATE 
    Qt6::Widgets 
    Qt6::Network 
    Qt6::Charts
)