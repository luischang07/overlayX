# OverlayX Controller Technical Documentation

The Controller is a Qt/QML application responsible for managing the lifecycle, configuration, and real-time state of the OverlayX engine.

## 🏗 Architecture

The controller follows an MVVM-inspired pattern where `AppBackend` (C++) serves as the primary bridge to the QML frontend.

### 1. Backend: `AppBackend`
- **Configuration Management**: Uses `IConfigRepository` to persist the `OverlayConfig` structure to JSON.
- **IPC (Inter-Process Communication)**: Implements Named Pipes to send serialized configurations to the `OverlayEngine`.
- **Hotkey Engine**: 
    - Runs a background polling loop using `GetAsyncKeyState`.
    - Handles **Global Toggle** (Show/Hide everything).
    - Handles **Instance Visibility** (Toggle specific crosshairs).
    - **Synchronization**: Automatically syncs hotkey changes between active instances and their source templates in `m_config.savedPresets`.
- **Lifecycle Management**: Handles the creation, movement, and deletion of crosshair instances.
- **Detection Controls**: Manages the `countdown` configuration (detection enabled, frequencies, and manual stop/reset signals).

### 2. Frontend: QML UI
- **`Main.qml`**: Root window and sidebar navigation.
- **`DesignerPage.qml`**: Detailed property editor for crosshair layers (Position, Size, Color, Thickness, etc.).
- **`PresetsPage.qml`**: Management of preset instances and on-screen position controls.
- **`HotkeysPage.qml`**: Interface for assigning and clearing hotkeys.

## 🔄 Reactivity & Synchronization

To maintain a clean separation between C++ and QML, the controller uses a "Tick-based" reactivity pattern:
- The backend maintains a `configUpdateTick` integer.
- Whenever a significant state change occurs (e.g., hotkey assigned, layer moved), `configUpdateTick` is incremented.
- QML components bind to this property to trigger re-evaluation of their visual state.

## ⌨️ Hotkey Implementation Details

### Assignment Flow
1. **Trigger**: User clicks "Assign" in `HotkeysPage.qml`.
2. **State**: `AppBackend` sets `m_listeningForHotkey = true` and stores the target `instanceId`.
3. **Polling**: `AppBackend::pollHotkeys()` detects the first valid keypress (ignoring `VK_LBUTTON` and `VK_ESCAPE` which cancel the action).
4. **Processing**: `setInstanceHotkey` is called.
5. **Persistence**: The hotkey is saved to both the active `OverlayInstance` and the source `PresetTemplate`.
6. **Notification**: `configUpdateTick` is incremented, and the UI reflects the new keybind.

## 🎯 Spike Detection Engine (v3.0)

The engine features a high-precision screen monitoring system implemented in `SpikeDetector.hpp`.

### 1. Detection Pipeline
The detector uses OpenCV to analyze screen regions in real-time:
- **Calibrated ROI**: Captures a specific region of interest (ROI) calibrated for 2560x1440, with automatic DPI scaling for other resolutions.
- **Redness Filter**: Isolate red pixels by calculating `R - max(G, B)` to distinguish icons from complex game backgrounds.
- **Edge Analysis**: Uses the **Canny Edge Detector** to identify sharp shapes, followed by a Gaussian blur for noise robustness.
- **Template Matching**: Employs `TM_CCORR_NORMED` with a custom mask to match the spike/defuse icon shape regardless of transparency.

### 2. State Management & Cooldown
To ensure reliable operation without false re-triggers:
- **45-Second Cooldown**: Once a spike is detected, the engine locks the detector for 45 seconds.
- **Manual Override**: If the user toggles the detection setting in the controller, the engine clears the cooldown and resets the detector state immediately.
- **Adaptive Polling**: The detector switches between an **Idle Frequency** (low CPU) and an **Active Frequency** (high responsiveness) based on screen activity detection.

## 📡 IPC Protocol
The controller communicates with the engine using a lightweight binary/JSON protocol over Named Pipes.
- **Pipe Name**: `\\.\pipe\OverlayXPipe`
- **Frequency**: Updates are sent on every configuration change with a small debounce to prevent flooding.
- **State Sync**: The engine sends runtime countdown states back to the controller to keep the UI synchronized.

## 📂 Key Files
- `src/app/AppBackend.hpp`: Core logic and state management.
- `src/qml/HotkeysPage.qml`: Hotkey management UI.
- `src/qml/PresetsPage.qml`: Instance management UI.
- `src/qml/DesignerPage.qml`: Layer editor UI.
- `../engine/src/infrastructure/SpikeDetector.hpp`: The v3.0 detection engine.
- `../engine/src/app/EngineApp.hpp`: Orchestration of detection and cooldown logic.
