# Generated from qardboard.pro.

cmake_minimum_required(VERSION 3.14)
project(qardboard LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

add_qt_gui_executable(qardboard
    abstractdeviceorientation.cpp abstractdeviceorientation.h
    dummydeviceorientation.cpp dummydeviceorientation.h
    main.cpp
)
target_link_libraries(qardboard PUBLIC
    Qt::3DCore
    Qt::3DExtras
    Qt::3DInput
    Qt::3DQuick
    Qt::3DQuickExtras
    Qt::3DRender
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(resources_resource_files
    "PlacedEntity.qml"
    "QardboardCamera.qml"
    "QardboardFrameGraph.qml"
    "QardboardRootEntity.qml"
    "QardboardScene3D.qml"
    "main.qml"
)

qt6_add_resources(qardboard "resources"
    PREFIX
        "/"
    FILES
        ${resources_resource_files}
)
set_source_files_properties("../exampleresources/assets/obj/ball.obj"
    PROPERTIES QT_RESOURCE_ALIAS "assets/obj/ball.obj"
)
set_source_files_properties("../exampleresources/assets/obj/material-sphere.obj"
    PROPERTIES QT_RESOURCE_ALIAS "assets/obj/material-sphere.obj"
)
set_source_files_properties("../exampleresources/assets/obj/plane-10x10.obj"
    PROPERTIES QT_RESOURCE_ALIAS "assets/obj/plane-10x10.obj"
)
set_source_files_properties("../exampleresources/assets/obj/toyplane.obj"
    PROPERTIES QT_RESOURCE_ALIAS "assets/obj/toyplane.obj"
)
set_source_files_properties("../exampleresources/assets/obj/trefoil.obj"
    PROPERTIES QT_RESOURCE_ALIAS "assets/obj/trefoil.obj"
)
set(obj_resource_files
    "assets/obj/ball.obj"
    "assets/obj/material-sphere.obj"
    "assets/obj/plane-10x10.obj"
    "assets/obj/toyplane.obj"
    "assets/obj/trefoil.obj"
)

qt6_add_resources(qardboard "obj"
    PREFIX
        "/"
    BASE
        "../exampleresources"
    FILES
        ${obj_resource_files}
)

if(IOS)
    target_sources(qardboard PUBLIC
        iosdeviceorientation.h iosdeviceorientation.mm iosdeviceorientation_p.h iosdeviceorientation_p.mm
    )

    target_link_libraries(qardboard PUBLIC
        Qt::Sensors
        Qt::SensorsPrivate
    )
endif()

install(TARGETS qardboard
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
