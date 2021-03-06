# Generated from qgltf.pro.

cmake_minimum_required(VERSION 3.14)
project(qgltf LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

add_qt_gui_executable(qgltf
    main.cpp
)
target_link_libraries(qgltf PUBLIC
    Qt::3DCore
    Qt::3DExtras
    3dquick
    Qt::3DQuickExtras
    Qt::3DRender
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(qgltf_example_resource_files
    "Scene.qml"
    "main.qml"
)

qt6_add_resources(qgltf "qgltf_example"
    PREFIX
        "/"
    FILES
        ${qgltf_example_resource_files}
)

install(TARGETS qgltf
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
