cmake_minimum_required(VERSION 3.15)

project(GstCelpPlugin VERSION 0.1.0 LANGUAGES C)

# --- Project Version Information ---
set(PROJECT_VERSION_RELEASE 0)
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION "${PROJECT_VERSION_RELEASE}.${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
message(STATUS "Configuring ${PROJECT_NAME} version ${PROJECT_VERSION}")

# --- Compiler and Build Options ---
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Find Dependencies ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED
    gstreamer-1.0>=1.6
    gstreamer-base-1.0>=1.6
    gstreamer-audio-1.0>=1.6
    gstreamer-plugins-base-1.0>=1.6
)

# --- Include Directories ---
include_directories(
    ${GSTREAMER_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/celp
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/common/inc
)

# --- Plugin Target ---
set(PLUGIN_NAME celp)
set(PLUGIN_TARGET_NAME gst${PLUGIN_NAME})
set(PLUGIN_SOURCES
    src/gstcelp.c
    src/gstcelpenc.c
    src/gstcelpenc.h
    src/gstcelpdec.c
    src/gstcelpdec.h
    celp/celp.c
)

add_library(${PLUGIN_TARGET_NAME} SHARED ${PLUGIN_SOURCES})
target_link_libraries(${PLUGIN_TARGET_NAME} ${GSTREAMER_LIBRARIES})
set_target_properties(${PLUGIN_TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

# --- Installation ---
include(GNUInstallDirs)
install(TARGETS ${PLUGIN_TARGET_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/gstreamer-1.0
)

message(STATUS "CELP plugin configuration complete. Build with 'cmake --build .'")