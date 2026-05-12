# Agent Implementation Protocol: Tactical Map Overlay (C++) 

## **Task Objective**
Develop a twin-process C++ application. 
* **Process A (Controller):** A standard Win32 window with a tabbed UI to manage crosshair settings, keybinds, and positioning.
* **Process B (Engine):** A lightweight, transparent, click-through overlay that renders the crosshair based on the Controller's settings.

---

## **1. Architecture: Twin-Process Model**
To ensure stability and anti-cheat compliance, the UI (Heavy) is separated from the Renderer (Light).
* **Communication:** Inter-Process Communication (IPC) via Shared Memory or a simple Named Pipe.
* **Data Structure:** `OverlayConfig { float x, y; int shapeID; int r, g, b, a; float thickness; int hotkey; }`

---

## **2. Sprint 1: Controller UI (Process A)**
* **Task 1.1: Tabbed Interface.** Implement three primary tabs:
    * **[General]:** Hotkey management and startup options.
    * **[Customize]:** Shape selection (Cross, Dot, Circle), RGBA color sliders, and thickness/size sliders.
    * **[Position]:** Nudge controls (Arrow keys) to fine-tune placement over the minimap.
* **Task 1.2: Persistence.** Implement Save/Load logic using a standard `.ini` or `.json` format.

---

## **3. Sprint 2: Overlay Engine (Process B)**
* **Task 2.1: Ghost Window.** Initialize a window with `WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST`.
* **Task 2.2: IPC Listener.** Implement a loop that checks for configuration updates from Process A.
* **Task 2.3: GDI+ Dynamic Drawing.** * **IF** `shapeID == 0` (Cross): Draw two lines using `Graphics::DrawLine`.
    * **IF** `shapeID == 1` (Dot): Draw a filled ellipse using `Graphics::FillEllipse`.
    * **IF** `shapeID == 2` (Circle): Draw an outline using `Graphics::DrawEllipse`.

---

## **4. Sprint 3: Interaction & Keybinds**
* **Task 3.1: Global Hotkey Listener.** Process A registers the user-defined keybind using `RegisterHotKey`.
* **Task 3.2: Mode Toggle.**
    * **Locked Mode:** Engine process has `WS_EX_TRANSPARENT` (Click-through).
    * **Edit Mode:** Controller sends signal to Engine to remove `WS_EX_TRANSPARENT` so the user can drag the overlay manually.

---

## **5. Agent Guardrails**
* **Anti-Cheat:** Use standard Win32 drawing. Do NOT use DirectX/Vulkan hooking. 
* **Performance:** Ensure Process B (Engine) uses `< 1%` CPU. Limit redraws to only when configuration changes or window is invalidated.
* **Safety:** Process A should be able to kill Process B on exit to prevent "orphan" overlays.

---

## **6. Reference Mapping (From User Images)**
* **Image 1 (UI):** Use as layout reference for the [General] tab buttons and keybind assignment.
* **Image 2/3 (Settings):** Use as logic reference for sliders (Gap, Size, Thickness, Opacity) and Shape selection.
