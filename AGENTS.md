# AGENTS.md — OverlayX Developer & Agent Guide

Welcome to the **OverlayX** codebase. This guide provides AI coding agents and human developers with essential context, architectural foundations, build instructions, and guidelines for working effectively with this project.

---

## 🧭 Knowledge Graph & Navigation (Graphify)

This project is mapped using `graphify`. The knowledge graph and reports reside in [graphify-out/](file:///c:/Users/luisc/Documents/c++/overlayX/graphify-out/).

- **Codebase queries**: Run `graphify query "<question>"` to inspect targeted relationships and subgraphs.
- **Explain concepts**: Run `graphify explain "<concept>"` for focused architecture/entity abstractions.
- **Inspect paths**: Run `graphify path "<SymbolA>" "<SymbolB>"` to trace dependencies across components.
- **After making code changes**: Run `graphify update .` to keep the AST knowledge graph up-to-date (no LLM key required).
- **Architecture summary**: See [graphify-out/GRAPH_REPORT.md](file:///c:/Users/luisc/Documents/c++/overlayX/graphify-out/GRAPH_REPORT.md).

---

## 🏛️ Architecture Overview

OverlayX is split into two primary communicating processes:

```
┌─────────────────────────────────┐           Windows Named Pipe           ┌────────────────────────────────┐
│      Controller (Qt 6 / QML)    │ ─────────────────────────────────────► │       Engine (Direct2D)        │
│                                 │        \\.\pipe\OverlayXPipe           │                                │
│  - AppBackend (C++)             │                                        │  - EngineApp (C++)             │
│  - JsonConfigRepository         │ ◄───────────────────────────────────── │  - OverlayWindow (Win32)       │
│  - NamedPipeServer              │       (Runtime state sync)             │  - Direct2DRenderer            │
│  - QML UI (Designer, Hotkeys)   │                                        │  - SpikeDetector (OpenCV)      │
└─────────────────────────────────┘                                        └────────────────────────────────┘
                 ▲                                                                         ▲
                 │                                                                         │
                 └─────────────────────────┬───────────────────────────────────────────────┘
                                           │
                           ┌───────────────────────────────┐
                           │   Core Shared Library / Headers│
                           │                               │
                           │  - OverlayConfig (Entities)   │
                           │  - Interfaces (IPC, Repo)     │
                           └───────────────────────────────┘
```

### 1. `controller/` (UI & Configuration Manager)
- **Framework**: Qt 6 Quick / QML.
- **Primary Backend**: [AppBackend.hpp](file:///c:/Users/luisc/Documents/c++/overlayX/controller/src/app/AppBackend.hpp).
  - Handles JSON configuration persistence ([JsonConfigRepository.hpp](file:///c:/Users/luisc/Documents/c++/overlayX/controller/src/infrastructure/JsonConfigRepository.hpp)).
  - Hosts [NamedPipeServer.hpp](file:///c:/Users/luisc/Documents/c++/overlayX/controller/src/infrastructure/NamedPipeServer.hpp) sending configs to the engine.
  - Background hotkey polling loop via Win32 `GetAsyncKeyState`.
  - Uses `configUpdateTick` integer increments for reactive synchronization with QML pages ([HotkeysPage.qml](file:///c:/Users/luisc/Documents/c++/overlayX/controller/src/qml/HotkeysPage.qml), [PresetsPage.qml](file:///c:/Users/luisc/Documents/c++/overlayX/controller/src/qml/PresetsPage.qml), [DesignerPage.qml](file:///c:/Users/luisc/Documents/c++/overlayX/controller/src/qml/DesignerPage.qml)).

### 2. `engine/` (Direct2D Render & Detection Engine)
- **Framework**: Native Win32 + Direct2D + OpenCV.
- **Coordinator**: [EngineApp.hpp](file:///c:/Users/luisc/Documents/c++/overlayX/engine/src/app/EngineApp.hpp).
- **Window**: [OverlayWindow.hpp](file:///c:/Users/luisc/Documents/c++/overlayX/engine/src/infrastructure/OverlayWindow.hpp) — transparent, layered, top-most click-through window.
- **Renderer**: [Direct2DRenderer.hpp](file:///c:/Users/luisc/Documents/c++/overlayX/engine/src/rendering/Direct2DRenderer.hpp) — hardware-accelerated rendering of stacked crosshair layers, countdown rings, and dynamic styles.
- **Spike Detector**: [SpikeDetector.hpp](file:///c:/Users/luisc/Documents/c++/overlayX/engine/src/infrastructure/SpikeDetector.hpp) — real-time computer vision pipeline (ROI calibration, redness extraction, Canny edge detection, template matching, 45-second cooldown cycle).
- **IPC Client**: [NamedPipeClient.hpp](file:///c:/Users/luisc/Documents/c++/overlayX/engine/src/infrastructure/NamedPipeClient.hpp).

### 3. `core/` (Shared Data Entities & Interfaces)
- **Config & Entities**: [OverlayConfig.hpp](file:///c:/Users/luisc/Documents/c++/overlayX/core/include/entities/OverlayConfig.hpp) (`OverlayConfig`, `Layer`, `CountdownConfig`, `Color`, `OverlayInstance`, `PresetTemplate`).
- **Interfaces**: [IIPCServer.hpp](file:///c:/Users/luisc/Documents/c++/overlayX/core/include/interfaces/IIPCServer.hpp), [IIPCClient.hpp](file:///c:/Users/luisc/Documents/c++/overlayX/core/include/interfaces/IIPCClient.hpp), [IConfigRepository.hpp](file:///c:/Users/luisc/Documents/c++/overlayX/core/include/interfaces/IConfigRepository.hpp), [IRenderer.hpp](file:///c:/Users/luisc/Documents/c++/overlayX/core/include/interfaces/IRenderer.hpp).

---

## 🛠️ Build & Development Commands

### Prerequisites
- Windows 10/11 (64-bit)
- MSVC (Visual Studio 2019/2022 C++ toolchain with C++20 support)
- CMake 3.20+
- Qt 6.x (Core, Gui, Qml, Quick)
- OpenCV 4.x

### CMake Build
From project root:
```powershell
# Configure build
cmake -B build -S .

# Compile target (Release or Debug)
cmake --build build --config Release
```

---

## 📌 Coding & Agent Guidelines

1. **Header-Only Infrastructure Patterns**:
   - Notice that much of `engine/` and `controller/infrastructure` is cleanly organized into header-based implementations (`.hpp`). Adhere to established encapsulation patterns when adding or refactoring classes.
2. **IPC Synchronization**:
   - Any modification to `OverlayConfig` in [OverlayConfig.hpp](file:///c:/Users/luisc/Documents/c++/overlayX/core/include/entities/OverlayConfig.hpp) must ensure `to_json` / `from_json` serialization stays consistent across both Controller and Engine.
   - When updating properties from the controller, ensure updates trigger IPC packet dispatch with appropriate debouncing.
3. **Thread Safety**:
   - `EngineApp` and `AppBackend` operate background threads (`std::jthread`) for IPC, hotkey polling, and spike detection. Guard shared state using corresponding `std::mutex` instances (`m_configMutex`, etc.).
4. **Resolution Independence**:
   - Coordinates and ROIs are calibrated for 1440p baseline with DPI and resolution scaling helpers in [SpikeDetector.hpp](file:///c:/Users/luisc/Documents/c++/overlayX/engine/src/infrastructure/SpikeDetector.hpp) and [Direct2DRenderer.hpp](file:///c:/Users/luisc/Documents/c++/overlayX/engine/src/rendering/Direct2DRenderer.hpp). Retain scaling logic when adding drawing or detection regions.
5. **Update Knowledge Graph**:
   - Run `graphify update .` after adding or refactoring code files.
