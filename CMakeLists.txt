cmake_minimum_required(VERSION 3.20)
project(OverlayXEngine LANGUAGES CXX)

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
# Spike detection is OPTIONAL - can be disabled if OpenCV isn't available
option(ENABLE_SPIKE_DETECTION "Enable spike icon detection" OFF)

if(ENABLE_SPIKE_DETECTION)
    find_package(OpenCV QUIET COMPONENTS core imgproc)
    if(OpenCV_FOUND)
        add_definitions(-DSPIKE_DETECTION_ENABLED)
        message(STATUS "Spike detection ENABLED (OpenCV found)")
    else()
        message(WARNING "OpenCV not found! Spike detection disabled. To enable, install OpenCV and set OpenCV_DIR.")
        set(ENABLE_SPIKE_DETECTION OFF)
    endif()
else()
    message(STATUS "Spike detection DISABLED (use -DENABLE_SPIKE_DETECTION=ON to enable)")
endif()

# ─── Engine Target (MSVC / Direct2D) ────────────────────────
add_executable(OverlayEngine
    engine/src/main.cpp
)

target_include_directories(OverlayEngine PRIVATE
    core/include
    shared/include
    engine/src
    ${JSON_INCLUDE_DIR}
)

target_link_libraries(OverlayEngine PRIVATE
    d2d1.lib
    dwrite.lib
    windowscodecs.lib
)

if(ENABLE_SPIKE_DETECTION)
    # Link OpenCV from vcpkg
    set(OPENCV_LIB_DIR "C:/vcpkg/packages/opencv4_x64-windows/lib")
    set(OPENCV_INCLUDE_DIR "C:/vcpkg/packages/opencv4_x64-windows/include")
    
    target_include_directories(OverlayEngine PRIVATE ${OPENCV_INCLUDE_DIR})
    target_link_directories(OverlayEngine PRIVATE ${OPENCV_LIB_DIR})
    target_link_libraries(OverlayEngine PRIVATE 
        opencv_core
        opencv_imgproc
        opencv_imgcodecs
        d3d11.lib
        dxgi.lib
    )
endif()
