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
