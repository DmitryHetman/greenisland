find_package(Qt5Gui REQUIRED COMPONENTS Private)
find_package(Qt5Qml REQUIRED COMPONENTS Private)
find_package(Qt5Quick REQUIRED COMPONENTS Private)

# Qt 5.5.x doesn't define Qt5Qml and Qt5Quick private
# include directories, so we have to fix it
foreach(_mod Qml Quick)
    foreach(_dir ${Qt5${_mod}_INCLUDE_DIRS})
        set(_path "${_dir}/Qt${_mod}/${Qt5${_mod}_VERSION_STRING}")
        if(EXISTS ${_path})
            list(APPEND Qt5${_mod}_PRIVATE_INCLUDE_DIRS ${_path})
        endif()

        set(_path "${_dir}/Qt${_mod}/${Qt5${_mod}_VERSION_STRING}/Qt${_mod}")
        if(EXISTS ${_path})
            list(APPEND Qt5${_mod}_PRIVATE_INCLUDE_DIRS ${_path})
        endif()
    endforeach()
endforeach()

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/waylandcompositor
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_BINARY_DIR}/src/waylandcompositor
    ${CMAKE_BINARY_DIR}/headers
    ${CMAKE_BINARY_DIR}/headers/GreenIsland
    ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
    ${Qt5Qml_PRIVATE_INCLUDE_DIRS}
    ${Qt5Quick_PRIVATE_INCLUDE_DIRS}
)

if(systemd_FOUND)
    include_directories(${systemd_INCLUDE_DIRS})
endif()

add_definitions(
    -Wno-deprecated-declarations
    -DQT_WAYLAND_COMPOSITOR_QUICK
)

include(GreenIslandServerConfigExtrasLocal.cmake)
include(GreenIslandServerMacros.cmake)

set(SOURCES
    serverlogging_p.cpp
    core/abstractplugin.cpp
    core/compositorsettings.cpp
    core/diagnostic_p.cpp
    core/homeapplication.cpp
    core/quickoutput.cpp
    input/keymap.cpp
    output/outputchangeset.cpp
    output/outputconfiguration.cpp
    output/outputmanagement.cpp
    output/quickoutputconfiguration.cpp
    screen/fakescreenbackend.cpp
    screen/nativescreenbackend.cpp
    screen/quickscreenmanager.cpp
    screen/screenbackend.cpp
    screen/screenmanager.cpp
    shell/clientwindow.cpp
    shell/clientwindowquickitem.cpp
    extensions/applicationmanager.cpp
    extensions/gtkshell.cpp
    extensions/screencaster.cpp
    extensions/screenshooter.cpp
)

greenisland_add_server_protocol(SOURCES
    PROTOCOL ${CMAKE_SOURCE_DIR}/data/protocols/wayland/wayland.xml
    BASENAME wayland
    PREFIX wl_
)
greenisland_add_server_protocol(SOURCES
    PROTOCOL ${CMAKE_SOURCE_DIR}/data/protocols/greenisland/greenisland.xml
    BASENAME greenisland
    PREFIX greenisland_
)
greenisland_add_server_protocol(SOURCES
    PROTOCOL ${CMAKE_SOURCE_DIR}/data/protocols/greenisland/greenisland-screencaster.xml
    BASENAME greenisland-screencaster
    PREFIX greenisland_
)
greenisland_add_server_protocol(SOURCES
    PROTOCOL ${CMAKE_SOURCE_DIR}/data/protocols/greenisland/greenisland-screenshooter.xml
    BASENAME greenisland-screenshooter
    PREFIX greenisland_
)
greenisland_add_server_protocol(SOURCES
    PROTOCOL ${CMAKE_SOURCE_DIR}/data/protocols/greenisland/greenisland-outputmanagement.xml
    BASENAME greenisland-outputmanagement
    PREFIX greenisland_
)
greenisland_add_server_protocol(SOURCES
    PROTOCOL ${CMAKE_SOURCE_DIR}/data/protocols/gtk/gtk-shell.xml
    BASENAME gtk
    PREFIX gtk_
)

add_library(GreenIslandServer SHARED ${SOURCES})
generate_export_header(GreenIslandServer EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/headers/GreenIsland/server/greenislandserver_export.h)
add_library(GreenIsland::Server ALIAS GreenIslandServer)

target_link_libraries(GreenIslandServer
    PUBLIC
        Qt5::Core
        Qt5::Gui
        Qt5::Quick
        Wayland::Server
        GreenIsland::Compositor
        GreenIsland::Platform
    PRIVATE
        Wayland::Cursor
        EGL::EGL
)

if(EGL_FOUND)
    if(${Qt5Gui_OPENGL_IMPLEMENTATION} STREQUAL "GL")
        target_link_libraries(GreenIslandServer PRIVATE ${OPENGL_LIBRARIES})
    else()
        target_link_libraries(GreenIslandServer PRIVATE ${OPENGLES_LIBRARIES})
    endif()
endif()

if(systemd_FOUND)
    target_link_libraries(GreenIslandServer PRIVATE ${systemd_LIBRARIES})
endif()

target_include_directories(
    GreenIslandServer
        INTERFACE
            "$<INSTALL_INTERFACE:${HAWAII_INCLUDEDIR}>"
            "$<INSTALL_INTERFACE:${GREENISLAND_INCLUDEDIR}>"
)

set_target_properties(GreenIslandServer PROPERTIES
    VERSION   ${PROJECT_VERSION}
    SOVERSION ${PROJECT_SOVERSION}
    EXPORT_NAME Server
)

ecm_generate_headers(GreenIslandServer_CamelCase_HEADERS
    HEADER_NAMES
        AbstractPlugin
        CompositorSettings
        HomeApplication
        QuickOutput
    PREFIX
        Server
    OUTPUT_DIR
        ${CMAKE_BINARY_DIR}/headers/GreenIsland
    REQUIRED_HEADERS
        GreenIslandServer_HEADERS
    RELATIVE
        core
)

ecm_generate_headers(GreenIslandServer_CamelCase_HEADERS
    HEADER_NAMES
        Keymap
    PREFIX
        Server
    OUTPUT_DIR
        ${CMAKE_BINARY_DIR}/headers/GreenIsland
    REQUIRED_HEADERS
        GreenIslandServer_HEADERS
    RELATIVE
        input
)

ecm_generate_headers(GreenIslandServer_CamelCase_HEADERS
    HEADER_NAMES
        OutputChangeset
        OutputConfiguration
        OutputManagement
        QuickOutputConfiguration
    PREFIX
        Server
    OUTPUT_DIR
        ${CMAKE_BINARY_DIR}/headers/GreenIsland
    REQUIRED_HEADERS
        GreenIslandServer_HEADERS
    RELATIVE
        output
)

ecm_generate_headers(GreenIslandServer_CamelCase_HEADERS
    HEADER_NAMES
        ApplicationManager
        GtkShell,GtkSurface
        Screencaster,Screencast
        Screenshooter,Screenshot
        TaskManager,TaskItem
    PREFIX
        Server
    OUTPUT_DIR
        ${CMAKE_BINARY_DIR}/headers/GreenIsland
    REQUIRED_HEADERS
        GreenIslandServer_HEADERS
    RELATIVE
        extensions
)

ecm_generate_headers(GreenIslandServer_CamelCase_HEADERS
    HEADER_NAMES
        QuickScreenManager
        ScreenBackend,Screen
        ScreenManager
    PREFIX
        Server
    OUTPUT_DIR
        ${CMAKE_BINARY_DIR}/headers/GreenIsland
    REQUIRED_HEADERS
        GreenIslandServer_HEADERS
    RELATIVE
        screen
)

ecm_generate_headers(GreenIslandServer_CamelCase_HEADERS
    HEADER_NAMES
        ClientWindow
        ClientWindowQuickItem
    PREFIX
        Server
    OUTPUT_DIR
        ${CMAKE_BINARY_DIR}/headers/GreenIsland
    REQUIRED_HEADERS
        GreenIslandServer_HEADERS
    RELATIVE
        shell
)

private_headers(GreenIslandServer_PRIVATE_HEADERS
    HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/extensions/applicationmanager_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/extensions/screencaster_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/extensions/screenshooter_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/input/keymap_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/output/outputchangeset_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/output/outputconfiguration_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/output/outputmanagement_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/output/quickoutputconfiguration_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/screen/screenbackend_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/screen/screenmanager_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/shell/clientwindow_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/shell/clientwindowquickitem_p.h
        ${CMAKE_CURRENT_BINARY_DIR}/qwayland-server-greenisland-outputmanagement.h
        ${CMAKE_CURRENT_BINARY_DIR}/qwayland-server-greenisland-screencaster.h
        ${CMAKE_CURRENT_BINARY_DIR}/qwayland-server-greenisland-screenshooter.h
        ${CMAKE_CURRENT_BINARY_DIR}/qwayland-server-gtk.h
        ${CMAKE_CURRENT_BINARY_DIR}/qwayland-server-greenisland.h
        ${CMAKE_CURRENT_BINARY_DIR}/wayland-greenisland-outputmanagement-server-protocol.h
        ${CMAKE_CURRENT_BINARY_DIR}/wayland-greenisland-screencaster-server-protocol.h
        ${CMAKE_CURRENT_BINARY_DIR}/wayland-greenisland-screenshooter-server-protocol.h
        ${CMAKE_CURRENT_BINARY_DIR}/wayland-gtk-shell-server-protocol.h
    OUTPUT_DIR
        ${CMAKE_BINARY_DIR}/headers/GreenIsland/server
)

install(TARGETS GreenIslandServer
        EXPORT GreenIslandServerTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES ${GreenIslandServer_CamelCase_HEADERS}
        DESTINATION ${GREENISLAND_INCLUDEDIR}/Server
        COMPONENT Devel)

install(
    FILES
        ${CMAKE_BINARY_DIR}/headers/GreenIsland/server/greenislandserver_export.h
        ${GreenIslandServer_HEADERS}
    DESTINATION
        ${GREENISLAND_INCLUDEDIR}/server
    COMPONENT
        Devel
)

# CMake package version
ecm_setup_version(${PROJECT_VERSION} VARIABLE_PREFIX GREENISLAND
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/GreenIslandServerConfigVersion.cmake"
                  SOVERSION ${PROJECT_SOVERSION})

# CMake package config
set(GreenIslandServer_CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/GreenIslandServer")
ecm_configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/GreenIslandServerConfig.cmake.in"
                                  "${CMAKE_CURRENT_BINARY_DIR}/GreenIslandServerConfig.cmake"
                                  INSTALL_DESTINATION ${GreenIslandServer_CMAKECONFIG_INSTALL_DIR})
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/GreenIslandServerConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/GreenIslandServerConfigVersion.cmake"
              "${CMAKE_CURRENT_SOURCE_DIR}/GreenIslandServerConfigExtras.cmake"
              "${CMAKE_CURRENT_SOURCE_DIR}/GreenIslandServerMacros.cmake"
        DESTINATION "${GreenIslandServer_CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel)
install(EXPORT GreenIslandServerTargets
        DESTINATION "${GreenIslandServer_CMAKECONFIG_INSTALL_DIR}"
        FILE GreenIslandServerTargets.cmake NAMESPACE GreenIsland::)

# qmake pri file
include(ECMGeneratePriFile)
ecm_generate_pri_file(BASE_NAME GreenIslandServer
                      LIB_NAME GreenIslandServer
                      DEPS "core gui quick GreenIslandCompositor"
                      FILENAME_VAR PRI_FILENAME
                      INCLUDE_INSTALL_DIR "${HAWAII_INCLUDEDIR} ${GREENISLAND_INCLUDEDIR}")
install(FILES ${PRI_FILENAME}
        DESTINATION ${ECM_MKSPECS_INSTALL_DIR})

# pkg-config file
include(ECMGeneratePkgConfigFile)
#ecm_generate_pkgconfig_file(BASE_NAME GreenIslandServer
#                            LIB_NAME GreenIslandServer
#                            DEPS "Qt5Core Qt5Gui Qt5Quick GreenIslandCompositor"
#                            FILENAME_VAR PKGCONFIG_FILENAME
#                            INCLUDE_INSTALL_DIR "${HAWAII_INCLUDEDIR} ${GREENISLAND_INCLUDEDIR}"
#                            INSTALL)
