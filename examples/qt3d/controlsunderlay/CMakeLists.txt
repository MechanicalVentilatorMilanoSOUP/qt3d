# Generated from controlsunderlay.pro.

cmake_minimum_required(VERSION 3.14)
project(controlsunderlay LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

add_qt_gui_executable(controlsunderlay
    main.cpp
)
target_link_libraries(controlsunderlay PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(controls_resource_files
    "Logo.qml"
    "Qt_logo.obj"
    "main.qml"
)

qt6_add_resources(controlsunderlay "controls"
    PREFIX
        "/"
    FILES
        ${controls_resource_files}
)

install(TARGETS controlsunderlay
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
