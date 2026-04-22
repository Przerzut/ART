cmake_minimum_required(VERSION 3.16)
project(AnalizatorTramwajowy VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON) # Niezbędne dla Qt (sygnały i sloty)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Szukanie bibliotek Qt w systemie
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Network Charts)

# Dodanie plików źródłowych
add_executable(AnalizatorTramwajowy
    main.cpp
    mainwindow.cpp
    mainwindow.h
    resources.qrc
)

# Łączenie bibliotek z Twoim programem
target_link_libraries(AnalizatorTramwajowy PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Charts
)