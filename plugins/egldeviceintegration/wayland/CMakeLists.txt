include_directories(
    ${CMAKE_BINARY_DIR}/headers
    ${CMAKE_BINARY_DIR}/headers/GreenIsland
    ${Qt5Core_PRIVATE_INCLUDE_DIRS}
    ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
    ${xkbcommon_INCLUDE_DIRS}
)

add_definitions(-DMESA_EGL_NO_X11_HEADERS)

set(SOURCES
    eglfswaylandblitter.cpp
    eglfswaylandcontext.cpp
    eglfswaylandinput.cpp
    eglfswaylandintegration.cpp
    eglfswaylandscreen.cpp
    eglfswaylandwindow.cpp
    main.cpp
)

include(ECMQtDeclareLoggingCategory)
ecm_qt_declare_logging_category(SOURCES
    HEADER eglfswaylandlogging.h
    IDENTIFIER GreenIsland::Platform::gLcEglFSWayland
    CATEGORY_NAME greenisland.qpa.wayland
    DEFAULT_SEVERITY Debug
)

add_library(wayland MODULE ${SOURCES})
target_link_libraries(wayland
    GreenIsland::Platform
    GreenIsland::Client
    Wayland::Egl
    ${xkbcommon_LIBRARIES}
)

if(${Qt5Gui_OPENGL_IMPLEMENTATION} STREQUAL "GL")
    target_link_libraries(wayland ${OPENGL_LIBRARIES})
else()
    target_link_libraries(wayland ${OPENGLES_LIBRARIES})
endif()

install(TARGETS wayland
        DESTINATION ${PLUGIN_INSTALL_DIR}/greenisland/egldeviceintegration)