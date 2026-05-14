# OverlayX

OverlayX is a high-performance, low-latency crosshair overlay system designed for Windows. It provides a sleek, modern interface to create, manage, and toggle complex crosshair configurations with multiple layers.

## 🚀 Features

- **Multi-Layer Designer**: Create complex crosshairs by stacking multiple layers with independent shapes, colors, and properties.
- **Modern UI**: Windows 11-inspired design built with Qt Quick/QML.
- **Real-Time Updates**: Changes in the controller are instantly reflected in the overlay engine via high-speed IPC.
- **Spike Detection (v3.0)**: High-precision screen detection engine (Canny + color-aware) that automatically triggers crosshairs or countdowns.
- **Robust Cooldown**: Intelligent 45-second detection cooldown with manual override to prevent double-triggers.
- **Advanced Hotkey System**: Assign hotkeys to toggle individual preset instances or the entire overlay globally.
- **Preset Management**: Save your designs as templates and re-use them across different sessions.
- **Independent Movement**: Move different crosshair instances independently on your screen.
- **Resolution Scaling**: Automatic coordinate scaling for 1080p, 1440p (2K), and 4K displays.

## 📂 Project Structure

- **`controller/`**: The management application (Qt/QML). This is where you design and control your overlays.
- **`engine/`**: The core rendering engine (Direct2D). A lightweight process that renders the actual crosshairs with minimal overhead.
- **`core/`**: Shared entities, configuration logic, and IPC interfaces used by both the controller and engine.
- **`shared/`**: Common utility functions and shared headers.

## 🛠️ Requirements

### Runtime Prerequisites (For Users)
- **Windows 10/11** (64-bit)
- **Administrator Privileges**: Required for screen capture and top-most overlay placement.
- **DirectX 11**: Compatible GPU for high-performance rendering.
- **Visual C++ Redistributable 2019/2022**: Ensure the latest [Microsoft VC++ redist](https://aka.ms/vs/17/release/vc_redist.x64.exe) is installed.

### Development Requirements (For Developers)
- **Qt 6.x**
- **CMake 3.16+**
- **MSVC 2019+** (with C++17 support)
- **OpenCV 4.x** (Required for Spike Detector features)

## 🏗️ Building

1. Clone the repository.
2. Create a build directory: `mkdir build && cd build`.
3. Run CMake: `cmake ..`.
4. Build the project: `cmake --build .`.

## 📜 License

This project is for educational/personal use. See the source headers for details.
