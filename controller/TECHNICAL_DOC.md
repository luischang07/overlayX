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

## 📡 IPC Protocol
The controller communicates with the engine using a lightweight binary/JSON protocol over Named Pipes.
- **Pipe Name**: `\\.\pipe\OverlayXPipe`
- **Frequency**: Updates are sent on every configuration change with a small debounce to prevent flooding.

## 📂 Key Files
- `src/app/AppBackend.hpp`: Core logic and state management.
- `src/qml/HotkeysPage.qml`: Hotkey management UI.
- `src/qml/PresetsPage.qml`: Instance management UI.
- `src/qml/DesignerPage.qml`: Layer editor UI.
