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
    ${CMAKE_BINARY_DIR}/headers
    ${CMAKE_BINARY_DIR}/headers/GreenIsland
    ${CMAKE_BINARY_DIR}/headers/GreenIsland/QtWaylandCompositor
    ${CMAKE_CURRENT_SOURCE_DIR}/compositor_api
    ${CMAKE_CURRENT_SOURCE_DIR}/extensions
    ${CMAKE_CURRENT_SOURCE_DIR}/global
    ${CMAKE_CURRENT_SOURCE_DIR}/wayland_wrapper
    ${CMAKE_CURRENT_SOURCE_DIR}/shared
    ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
    ${Qt5Qml_PRIVATE_INCLUDE_DIRS}
    ${Qt5Quick_PRIVATE_INCLUDE_DIRS}
    ${xkbcommon_INCLUDE_DIRS}
)

include(${CMAKE_SOURCE_DIR}/src/server/GreenIslandServerConfigExtrasLocal.cmake)
include(${CMAKE_SOURCE_DIR}/src/server/GreenIslandServerMacros.cmake)

set(SOURCES
    compositor_api/qwaylandbufferref.cpp
    compositor_api/qwaylandclient.cpp
    compositor_api/qwaylandcompositor.cpp
    compositor_api/qwaylanddestroylistener.cpp
    compositor_api/qwaylanddrag.cpp
    compositor_api/qwaylandinputmethodcontrol.cpp
    compositor_api/qwaylandkeyboard.cpp
    compositor_api/qwaylandoutput.cpp
    compositor_api/qwaylandpointer.cpp
    compositor_api/qwaylandquickcompositor.cpp
    compositor_api/qwaylandquickitem.cpp
    compositor_api/qwaylandquickoutput.cpp
    compositor_api/qwaylandquicksurface.cpp
    compositor_api/qwaylandresource.cpp
    compositor_api/qwaylandseat.cpp
    compositor_api/qwaylandsurface.cpp
    compositor_api/qwaylandsurfacegrabber.cpp
    compositor_api/qwaylandtouch.cpp
    compositor_api/qwaylandview.cpp
    extensions/qwaylandqtwindowmanager.cpp
    extensions/qwaylandquickshellsurfaceitem.cpp
    extensions/qwaylandtextinput.cpp
    extensions/qwaylandtextinputmanager.cpp
    extensions/qwaylandwlshell.cpp
    extensions/qwaylandwlshellintegration.cpp
    extensions/qwaylandxdgshell.cpp
    extensions/qwaylandxdgshellintegration.cpp
    extensions/qwaylandshell.cpp
    extensions/qwaylandshellsurface.cpp
    extensions/qwlextendedsurface.cpp
    extensions/qwlqtkey.cpp
    extensions/qwlqttouch.cpp
    global/qwaylandcompositorextension.cpp
    hardware_integration/qwlclientbufferintegration.cpp
    hardware_integration/qwlclientbufferintegrationfactory.cpp
    hardware_integration/qwlclientbufferintegrationplugin.cpp
    hardware_integration/qwlhwintegration.cpp
    hardware_integration/qwlserverbufferintegration.cpp
    hardware_integration/qwlserverbufferintegrationfactory.cpp
    hardware_integration/qwlserverbufferintegrationplugin.cpp
    shared/qwaylandinputmethodeventbuilder.cpp
    shared/qwaylandmimehelper.cpp
    shared/qwaylandxkb.cpp
    wayland_wrapper/qwldatadevice.cpp
    wayland_wrapper/qwldatadevicemanager.cpp
    wayland_wrapper/qwldataoffer.cpp
    wayland_wrapper/qwldatasource.cpp
    wayland_wrapper/qwlregion.cpp
    wayland_wrapper/qwlsurfacebuffer.cpp
)

if(${Qt5Gui_OPENGL_IMPLEMENTATION} STREQUAL "GL")
    add_definitions(-DQT_WAYLAND_COMPOSITOR_GL)
endif()

add_definitions(
    -DQT_BUILD_WAYLANDCOMPOSITOR_LIB
    -DQT_WAYLAND_COMPOSITOR_QUICK
)

greenisland_add_server_protocol(SOURCES
    PROTOCOL ${CMAKE_SOURCE_DIR}/data/protocols/qt/surface-extension.xml
    BASENAME surface-extension
    PREFIX qt_
)
greenisland_add_server_protocol(SOURCES
    PROTOCOL ${CMAKE_SOURCE_DIR}/data/protocols/qt/touch-extension.xml
    BASENAME touch-extension
    PREFIX qt_
)
greenisland_add_server_protocol(SOURCES
    PROTOCOL ${CMAKE_SOURCE_DIR}/data/protocols/qt/qtkey-extension.xml
    BASENAME qtkey-extension
    PREFIX qt_
)
greenisland_add_server_protocol(SOURCES
    PROTOCOL ${CMAKE_SOURCE_DIR}/data/protocols/qt/qt-windowmanager.xml
    BASENAME qt-windowmanager
    PREFIX qt_
)
greenisland_add_server_protocol(SOURCES
    PROTOCOL ${CMAKE_SOURCE_DIR}/data/protocols/qt/hardware-integration.xml
    BASENAME hardware-integration
    PREFIX qt_
)
greenisland_add_server_protocol(SOURCES
    PROTOCOL ${CMAKE_SOURCE_DIR}/data/protocols/wayland/wayland.xml
    BASENAME wayland
    PREFIX wl_
)
greenisland_add_server_protocol(SOURCES
    PROTOCOL ${CMAKE_SOURCE_DIR}/data/protocols/wayland/text-input-unstable-v2.xml
    BASENAME text-input-unstable-v2
    PREFIX wl_
)
greenisland_add_server_protocol(SOURCES
    PROTOCOL ${CMAKE_SOURCE_DIR}/data/protocols/wayland/xdg-shell.xml
    BASENAME xdg-shell
)

add_library(GreenIslandCompositor SHARED ${SOURCES})
add_library(GreenIsland::Compositor ALIAS GreenIslandCompositor)

target_link_libraries(GreenIslandCompositor
    PUBLIC
        Qt5::Core
        Qt5::Gui
        Qt5::Qml
        Qt5::Quick
        Wayland::Server
    PRIVATE
        ${xkbcommon_LIBRARIES}
)

if(${Qt5Gui_OPENGL_IMPLEMENTATION} STREQUAL "GL")
    target_link_libraries(GreenIslandCompositor PUBLIC ${OPENGL_LIBRARIES})
else()
    target_link_libraries(GreenIslandCompositor PUBLIC ${OPENGLES_LIBRARIES})
endif()

target_include_directories(
    GreenIslandCompositor
        PUBLIC
            "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/headers/GreenIsland>"
        INTERFACE
            "$<INSTALL_INTERFACE:${HAWAII_INCLUDEDIR}>"
            "$<INSTALL_INTERFACE:${GREENISLAND_INCLUDEDIR}>"
)

set_target_properties(GreenIslandCompositor PROPERTIES
    VERSION   ${PROJECT_VERSION}
    SOVERSION ${PROJECT_SOVERSION}
    EXPORT_NAME Compositor
)

generate_headers(GreenIslandCompositor_CamelCase_HEADERS
    HEADER_NAMES
        QWaylandBufferRef
        QWaylandClient
        QWaylandCompositor
        QWaylandDestroyListener
        QWaylandDrag
        QWaylandInputMethodControl
        QWaylandKeyboard,QWaylandKeymap
        QWaylandOutput
        QWaylandPointer
        QWaylandQuickCompositor
        QWaylandQuickItem
        QWaylandQuickOutput
        QWaylandQuickSurface
        QWaylandResource
        QWaylandSeat
        QWaylandSurface,QWaylandSurfaceRole
        QWaylandSurfaceGrabber
        QWaylandTouch
        QWaylandView
    PREFIX
        QtWaylandCompositor
    OUTPUT_DIR
        ${CMAKE_BINARY_DIR}/headers/GreenIsland
    REQUIRED_HEADERS
        GreenIslandCompositor_HEADERS
    RELATIVE
        compositor_api
)

generate_headers(GreenIslandCompositor_CamelCase_HEADERS
    HEADER_NAMES
        QWaylandQtWindowManager
        QWaylandQuickShellSurfaceItem
        QWaylandShell
        QWaylandShellSurface
        QWaylandTextInput
        QWaylandTextInputManager
        QWaylandWlShell,QWaylandWlShellSurface
        QWaylandXdgShell,QWaylandXdgSurface,QWaylandXdgPopup
    PREFIX
        QtWaylandCompositor
    OUTPUT_DIR
        ${CMAKE_BINARY_DIR}/headers/GreenIsland
    REQUIRED_HEADERS
        GreenIslandCompositor_HEADERS
    RELATIVE
        extensions
)

generate_headers(GreenIslandCompositor_CamelCase_HEADERS
    HEADER_NAMES
        QWaylandCompositorExtension,QWaylandCompositorExtensionTemplate,QWaylandObject
        QWaylandExport
        QWaylandQuickExtension
    PREFIX
        QtWaylandCompositor
    OUTPUT_DIR
        ${CMAKE_BINARY_DIR}/headers/GreenIsland
    REQUIRED_HEADERS
        GreenIslandCompositor_HEADERS
    RELATIVE
        global
)

private_headers(GreenIslandCompositor_PRIVATE_HEADERS
    HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/compositor_api/qwaylandcompositor_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/compositor_api/qwaylanddestroylistener_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/compositor_api/qwaylandinputmethodcontrol_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/compositor_api/qwaylandkeyboard_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/compositor_api/qwaylandoutput_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/compositor_api/qwaylandpointer_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/compositor_api/qwaylandquickitem_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/compositor_api/qwaylandseat_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/compositor_api/qwaylandsurface_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/compositor_api/qwaylandtouch_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/compositor_api/qwaylandview_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/extensions/qwaylandqtwindowmanager_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/extensions/qwaylandquickshellsurfaceitem_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/extensions/qwaylandtextinputmanager_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/extensions/qwaylandtextinput_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/extensions/qwaylandshell_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/extensions/qwaylandwlshellintegration_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/extensions/qwaylandwlshell_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/extensions/qwaylandxdgshellintegration_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/extensions/qwaylandxdgshell_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/extensions/qwlextendedsurface_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/extensions/qwlqtkey_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/extensions/qwlqttouch_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/global/qwaylandcompositorextension_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/hardware_integration/qwlclientbufferintegrationfactory_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/hardware_integration/qwlclientbufferintegration_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/hardware_integration/qwlclientbufferintegrationplugin_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/hardware_integration/qwlhwintegration_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/hardware_integration/qwlserverbufferintegrationfactory_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/hardware_integration/qwlserverbufferintegration_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/hardware_integration/qwlserverbufferintegrationplugin_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/shared/qwaylandinputmethodeventbuilder_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/shared/qwaylandmimehelper_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/shared/qwaylandshmformathelper_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/shared/qwaylandxkb_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland_wrapper/qwldatadevicemanager_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland_wrapper/qwldatadevice_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland_wrapper/qwldataoffer_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland_wrapper/qwldatasource_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland_wrapper/qwlregion_p.h
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland_wrapper/qwlsurfacebuffer_p.h
        ${CMAKE_CURRENT_BINARY_DIR}/qwayland-server-hardware-integration.h
        ${CMAKE_CURRENT_BINARY_DIR}/qwayland-server-text-input-unstable-v2.h
        ${CMAKE_CURRENT_BINARY_DIR}/qwayland-server-qtkey-extension.h
        ${CMAKE_CURRENT_BINARY_DIR}/qwayland-server-surface-extension.h
        ${CMAKE_CURRENT_BINARY_DIR}/qwayland-server-text.h
        ${CMAKE_CURRENT_BINARY_DIR}/qwayland-server-touch-extension.h
        ${CMAKE_CURRENT_BINARY_DIR}/qwayland-server-wayland.h
        ${CMAKE_CURRENT_BINARY_DIR}/qwayland-server-qt-windowmanager.h
        ${CMAKE_CURRENT_BINARY_DIR}/qwayland-server-xdg-shell.h
        ${CMAKE_CURRENT_BINARY_DIR}/wayland-hardware-integration-client-protocol.h
        ${CMAKE_CURRENT_BINARY_DIR}/wayland-hardware-integration-server-protocol.h
        ${CMAKE_CURRENT_BINARY_DIR}/wayland-input-method-client-protocol.h
        ${CMAKE_CURRENT_BINARY_DIR}/wayland-input-method-server-protocol.h
        ${CMAKE_CURRENT_BINARY_DIR}/wayland-qtkey-extension-client-protocol.h
        ${CMAKE_CURRENT_BINARY_DIR}/wayland-qtkey-extension-server-protocol.h
        ${CMAKE_CURRENT_BINARY_DIR}/wayland-surface-extension-client-protocol.h
        ${CMAKE_CURRENT_BINARY_DIR}/wayland-surface-extension-server-protocol.h
        ${CMAKE_CURRENT_BINARY_DIR}/wayland-text-input-unstable-v2-client-protocol.h
        ${CMAKE_CURRENT_BINARY_DIR}/wayland-text-input-unstable-v2-server-protocol.h
        ${CMAKE_CURRENT_BINARY_DIR}/wayland-touch-extension-client-protocol.h
        ${CMAKE_CURRENT_BINARY_DIR}/wayland-touch-extension-server-protocol.h
        ${CMAKE_CURRENT_BINARY_DIR}/wayland-wayland-client-protocol.h
        ${CMAKE_CURRENT_BINARY_DIR}/wayland-wayland-server-protocol.h
        ${CMAKE_CURRENT_BINARY_DIR}/wayland-qt-windowmanager-client-protocol.h
        ${CMAKE_CURRENT_BINARY_DIR}/wayland-qt-windowmanager-server-protocol.h
        ${CMAKE_CURRENT_BINARY_DIR}/wayland-xdg-shell-client-protocol.h
        ${CMAKE_CURRENT_BINARY_DIR}/wayland-xdg-shell-server-protocol.h
    OUTPUT_DIR
        ${CMAKE_BINARY_DIR}/headers/GreenIsland/QtWaylandCompositor
)

install(TARGETS GreenIslandCompositor
        EXPORT GreenIslandCompositorTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(
    FILES
        ${GreenIslandCompositor_CamelCase_HEADERS}
        ${GreenIslandCompositor_HEADERS}
    DESTINATION
        ${GREENISLAND_INCLUDEDIR}/QtWaylandCompositor
    COMPONENT
        Devel
)

# CMake package version
ecm_setup_version(${PROJECT_VERSION} VARIABLE_PREFIX GREENISLAND
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/GreenIslandCompositorConfigVersion.cmake"
                  SOVERSION ${PROJECT_SOVERSION})

# CMake package config
set(GreenIslandCompositor_CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/GreenIslandCompositor")
ecm_configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/GreenIslandCompositorConfig.cmake.in"
                                  "${CMAKE_CURRENT_BINARY_DIR}/GreenIslandCompositorConfig.cmake"
                                  INSTALL_DESTINATION ${GreenIslandCompositor_CMAKECONFIG_INSTALL_DIR})
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/GreenIslandCompositorConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/GreenIslandCompositorConfigVersion.cmake"
        DESTINATION "${GreenIslandCompositor_CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel)
install(EXPORT GreenIslandCompositorTargets
        DESTINATION "${GreenIslandCompositor_CMAKECONFIG_INSTALL_DIR}"
        FILE GreenIslandCompositorTargets.cmake NAMESPACE GreenIsland::)

# qmake pri file
include(ECMGeneratePriFile)
ecm_generate_pri_file(BASE_NAME GreenIslandCompositor
                      LIB_NAME GreenIslandCompositor
                      DEPS "core gui quick"
                      FILENAME_VAR PRI_FILENAME
                      INCLUDE_INSTALL_DIR "${HAWAII_INCLUDEDIR} ${GREENISLAND_INCLUDEDIR}")
install(FILES ${PRI_FILENAME}
        DESTINATION ${ECM_MKSPECS_INSTALL_DIR})

# pkg-config file
include(ECMGeneratePkgConfigFile)
#ecm_generate_pkgconfig_file(BASE_NAME GreenIslandCompositor
#                            LIB_NAME GreenIslandCompositor
#                            DEPS "Qt5Core Qt5Gui Qt5Quick"
#                            FILENAME_VAR PKGCONFIG_FILENAME
#                            INCLUDE_INSTALL_DIR "${HAWAII_INCLUDEDIR} ${GREENISLAND_INCLUDEDIR}"
#                            INSTALL)
