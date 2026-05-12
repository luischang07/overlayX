cmake_minimum_required(VERSION 3.20)
project(OverlayController LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

add_definitions(-DUNICODE -D_UNICODE)

# ─── Qt 6 ───────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Quick Qml)

# ─── nlohmann/json ──────────────────────────────────────────
set(JSON_INCLUDE_DIR "${CMAKE_BINARY_DIR}/deps")
file(MAKE_DIRECTORY "${JSON_INCLUDE_DIR}/nlohmann")
if(NOT EXISTS "${JSON_INCLUDE_DIR}/nlohmann/json.hpp")
    file(DOWNLOAD
        "https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp"
        "${JSON_INCLUDE_DIR}/nlohmann/json.hpp"
        SHOW_PROGRESS
    )
endif()

# ─── Controller Executable ──────────────────────────────────
qt_add_executable(OverlayController
    src/main.cpp
    src/app/AppBackend.hpp
)

qt_add_qml_module(OverlayController
    URI OverlayX
    VERSION 1.0
    QML_FILES
        src/qml/Main.qml
        src/qml/Sidebar.qml
        src/qml/OverviewPage.qml
        src/qml/DesignerPage.qml
        src/qml/PresetsPage.qml
        src/qml/HotkeysPage.qml
        src/qml/SettingsPage.qml
        src/qml/CountdownPage.qml
)

target_include_directories(OverlayController PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${JSON_INCLUDE_DIR}
)

target_link_libraries(OverlayController PRIVATE
    Qt6::Quick
    Qt6::Qml
)

# Windows: suppress console window in release
# set_target_properties(OverlayController PROPERTIES
#     WIN32_EXECUTABLE TRUE
# )
