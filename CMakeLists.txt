cmake_minimum_required(VERSION 3.20)
project(OverlayXEngine LANGUAGES CXX)
set(PORTABLE_DIR "${CMAKE_SOURCE_DIR}/portable")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Unicode for Win32
add_definitions(-DUNICODE -D_UNICODE)

# ─── nlohmann/json (single-header download) ─────────────────
set(JSON_INCLUDE_DIR "${CMAKE_BINARY_DIR}/deps")
file(MAKE_DIRECTORY "${JSON_INCLUDE_DIR}/nlohmann")
if(NOT EXISTS "${JSON_INCLUDE_DIR}/nlohmann/json.hpp")
    file(DOWNLOAD
        "https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp"
        "${JSON_INCLUDE_DIR}/nlohmann/json.hpp"
        SHOW_PROGRESS
    )
endif()

# ─── OpenCV (for spike detection) ──────────────────────────
option(ENABLE_SPIKE_DETECTION "Enable spike icon detection" ON)

if(ENABLE_SPIKE_DETECTION)
    find_package(OpenCV QUIET COMPONENTS core imgproc)
    
    # Check for MinGW compatibility if found via vcpkg
    if(OpenCV_FOUND AND MINGW AND OpenCV_DIR MATCHES "x64-windows")
        message(WARNING "OpenCV found in vcpkg (x64-windows) is for MSVC and incompatible with MinGW. Disabling spike detection.")
        set(OpenCV_FOUND OFF)
        set(ENABLE_SPIKE_DETECTION OFF)
    endif()

    if(OpenCV_FOUND)
        add_definitions(-DSPIKE_DETECTION_ENABLED)
        message(STATUS "Spike detection ENABLED (OpenCV found)")
    else()
        message(WARNING "OpenCV not found or incompatible! Spike detection disabled. To enable, install OpenCV for MinGW.")
        set(ENABLE_SPIKE_DETECTION OFF)
    endif()
endif()

# ─── Include Subdirectories ─────────────────────────────────
# Build the controller alongside the engine
add_subdirectory(controller)

# ─── Engine Target (MSVC / Direct2D) ────────────────────────
add_executable(OverlayEngine
    engine/src/main.cpp
)
# Hide console window
# set_target_properties(OverlayEngine PROPERTIES WIN32_EXECUTABLE ON)

target_include_directories(OverlayEngine PRIVATE
    core/include
    shared/include
    engine/src
    ${JSON_INCLUDE_DIR}
)

target_link_libraries(OverlayEngine PRIVATE
    d2d1
    dwrite
    windowscodecs
)

if(ENABLE_SPIKE_DETECTION)
    # Link OpenCV (vcpkg handles this if toolchain is correct, but keeping paths as backup)
    if(NOT OpenCV_FOUND)
        set(OPENCV_LIB_DIR "C:/vcpkg/packages/opencv4_x64-windows/lib")
        set(OPENCV_INCLUDE_DIR "C:/vcpkg/packages/opencv4_x64-windows/include")
        target_include_directories(OverlayEngine PRIVATE ${OPENCV_INCLUDE_DIR})
        target_link_directories(OverlayEngine PRIVATE ${OPENCV_LIB_DIR})
    endif()
    
    target_link_libraries(OverlayEngine PRIVATE 
        opencv_core
        opencv_imgproc
        opencv_imgcodecs
        d3d11
        dxgi
    )
endif()

# Auto-update portable folder
if(EXISTS "${PORTABLE_DIR}")
    add_custom_command(TARGET OverlayEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:OverlayEngine> "${PORTABLE_DIR}/OverlayEngine.exe"
        COMMENT "Updating ${PORTABLE_DIR}/OverlayEngine.exe"
    )
endif()

# ─── Unified Launcher ───────────────────────────────────────
add_executable(OverlayX
    launcher/main.cpp
)
set_target_properties(OverlayX PROPERTIES WIN32_EXECUTABLE ON)
if(MSVC)
    target_link_options(OverlayX PRIVATE "/MANIFESTUAC:level='requireAdministrator'")
else()
    # For MinGW, we use a resource file to embed the manifest
    target_sources(OverlayX PRIVATE launcher/launcher.rc)
endif()

# Auto-update portable folder
if(EXISTS "${PORTABLE_DIR}")
    add_custom_command(TARGET OverlayX POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:OverlayX> "${PORTABLE_DIR}/OverlayX.exe"
        COMMENT "Updating ${PORTABLE_DIR}/OverlayX.exe"
    )
endif()

# ─── Installation & Packaging ───────────────────────────────
include(GNUInstallDirs)

# Install Executables (Engine, Launcher, and Controller)
install(TARGETS OverlayX OverlayEngine OverlayController
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install Assets
install(FILES "spike.png"
    DESTINATION "bin/assets"
)

# CPack Configuration
set(CPACK_PACKAGE_NAME "OverlayX")
set(CPACK_PACKAGE_VENDOR "OverlayX Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Professional Gaming Overlay for Spike Detection")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "OverlayX")

# NSIS is the standard Windows installer generator (includes uninstaller by default)
set(CPACK_GENERATOR "NSIS")
set(CPACK_NSIS_DISPLAY_NAME "OverlayX")
set(CPACK_NSIS_PACKAGE_NAME "OverlayX")
set(CPACK_NSIS_MODIFY_PATH ON)
# Ensure it installs to Program Files by default (standard CPack behavior for NSIS)

include(CPack)

# ─── Update Portable Folder Automatically (Defined near target definitions) ───
