# OverlayX

OverlayX is a high-performance, low-latency crosshair overlay system designed for Windows. It provides a sleek, modern interface to create, manage, and toggle complex crosshair configurations with multiple layers.

## 🚀 Features

- **Multi-Layer Designer**: Create complex crosshairs by stacking multiple layers with independent shapes, colors, and properties.
- **Modern UI**: Windows 11-inspired design built with Qt Quick/QML.
- **Real-Time Updates**: Changes in the controller are instantly reflected in the overlay engine via high-speed IPC.
- **Advanced Hotkey System**: Assign hotkeys to toggle individual preset instances or the entire overlay globally.
- **Preset Management**: Save your designs as templates and re-use them across different sessions.
- **Independent Movement**: Move different crosshair instances independently on your screen.

## 📂 Project Structure

- **`controller/`**: The management application (Qt/QML). This is where you design and control your overlays.
- **`engine/`**: The core rendering engine (Direct2D). A lightweight process that renders the actual crosshairs with minimal overhead.
- **`core/`**: Shared entities, configuration logic, and IPC interfaces used by both the controller and engine.
- **`shared/`**: Common utility functions and shared headers.

## 🛠️ Requirements

- Windows 10/11
- Qt 6.x
- CMake 3.16+
- MSVC 2019+ (with C++17 support)

## 🏗️ Building

1. Clone the repository.
2. Create a build directory: `mkdir build && cd build`.
3. Run CMake: `cmake ..`.
4. Build the project: `cmake --build .`.

## 📜 License

This project is for educational/personal use. See the source headers for details.
