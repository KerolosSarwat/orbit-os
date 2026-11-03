cmake_minimum_required(VERSION 3.21)

project(orbit-shell VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Gui)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(orbit-shell
    src/main.cpp
    src/AppController.cpp
    src/AppController.h
    src/SystemMonitor.cpp
    src/SystemMonitor.h
)

qt_add_qml_module(orbit-shell
    URI Orbit.Shell
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

target_link_libraries(orbit-shell PRIVATE
    Qt6::Quick
    Qt6::Gui
)

install(TARGETS orbit-shell
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)
