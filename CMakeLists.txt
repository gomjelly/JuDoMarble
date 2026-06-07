cmake_minimum_required(VERSION 3.20)
project(JuDoMarble VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(JuDoMarble
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/EditorCanvas.h
    src/EditorCanvas.cpp
    src/PolygonItem.h
    src/SimulationWindow.h
    src/SimulationWindow.cpp
)

target_link_libraries(JuDoMarble PRIVATE Qt6::Widgets)

set_target_properties(JuDoMarble PROPERTIES
    WIN32_EXECUTABLE TRUE
)
