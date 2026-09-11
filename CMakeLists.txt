cmake_minimum_required(VERSION 3.16)
project(radialmenu LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 QUIET COMPONENTS Widgets)
if(NOT Qt6_FOUND)
    find_package(Qt5 5.12 REQUIRED COMPONENTS Widgets)
    set(QT_LIBS Qt5::Widgets)
else()
    set(QT_LIBS Qt6::Widgets)
endif()

add_executable(radialmenu
    src/main.cpp
    src/config.cpp
    src/uinputkeyboard.cpp
    src/evdevreader.cpp
    src/radialoverlay.cpp
)

target_link_libraries(radialmenu PRIVATE ${QT_LIBS})
target_include_directories(radialmenu PRIVATE include)
