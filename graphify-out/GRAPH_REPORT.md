# Graph Report - overlayX  (2026-10-27)

## Corpus Check
- 25 files · ~16,767 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 579 nodes · 901 edges · 31 communities (27 shown, 4 thin omitted)
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS · INFERRED: 1 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `20ee1c3a`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- [[_COMMUNITY_Community 0|Community 0]]
- [[_COMMUNITY_Community 1|Community 1]]
- [[_COMMUNITY_Community 2|Community 2]]
- [[_COMMUNITY_Community 3|Community 3]]
- [[_COMMUNITY_Community 4|Community 4]]
- [[_COMMUNITY_Community 6|Community 6]]
- [[_COMMUNITY_Community 7|Community 7]]
- [[_COMMUNITY_Community 8|Community 8]]
- [[_COMMUNITY_Community 9|Community 9]]
- [[_COMMUNITY_Community 10|Community 10]]
- [[_COMMUNITY_Community 11|Community 11]]
- [[_COMMUNITY_Community 12|Community 12]]
- [[_COMMUNITY_Community 13|Community 13]]
- [[_COMMUNITY_Community 14|Community 14]]
- [[_COMMUNITY_Community 15|Community 15]]
- [[_COMMUNITY_Community 16|Community 16]]
- [[_COMMUNITY_Community 17|Community 17]]
- [[_COMMUNITY_Community 18|Community 18]]
- [[_COMMUNITY_Community 19|Community 19]]
- [[_COMMUNITY_Community 20|Community 20]]
- [[_COMMUNITY_Community 21|Community 21]]
- [[_COMMUNITY_Community 22|Community 22]]
- [[_COMMUNITY_Community 23|Community 23]]
- [[_COMMUNITY_Community 24|Community 24]]
- [[_COMMUNITY_Community 25|Community 25]]
- [[_COMMUNITY_Community 26|Community 26]]
- [[_COMMUNITY_Community 27|Community 27]]
- [[_COMMUNITY_Community 29|Community 29]]
- [[_COMMUNITY_Community 30|Community 30]]

## God Nodes (most connected - your core abstractions)
1. `AppBackend` - 158 edges
2. `Q_INVOKABLE` - 54 edges
3. `Direct2DRenderer` - 43 edges
4. `QString` - 29 edges
5. `Layer` - 26 edges
6. `CountdownConfig` - 25 edges
7. `OverlayConfig` - 25 edges
8. `SpikeDetector` - 22 edges
9. `NamedPipeClient` - 21 edges
10. `OverlayWindow` - 19 edges

## Surprising Connections (you probably didn't know these)
- `AppBackend` --references--> `IConfigRepository`  [EXTRACTED]
  controller/src/app/AppBackend.hpp → controller/src/app/AppBackend.hpp  _Bridges community 0 → community 2_
- `AppBackend` --references--> `string`  [EXTRACTED]
  controller/src/app/AppBackend.hpp → controller/src/app/AppBackend.hpp  _Bridges community 0 → community 23_
- `AppBackend` --references--> `Q_INVOKABLE`  [EXTRACTED]
  controller/src/app/AppBackend.hpp → controller/src/app/AppBackend.hpp  _Bridges community 0 → community 10_
- `CountdownConfig` --references--> `Color`  [EXTRACTED]
  core/include/entities/OverlayConfig.hpp → core/include/entities/OverlayConfig.hpp  _Bridges community 27 → community 11_
- `from_json()` --references--> `Color`  [EXTRACTED]
  core/include/entities/OverlayConfig.hpp → core/include/entities/OverlayConfig.hpp  _Bridges community 27 → community 16_

## Import Cycles
- None detected.

## Communities (31 total, 4 thin omitted)

### Community 0 - "Community 0"
Cohesion: 0.04
Nodes (33): AppBackend, configChanged, engineStatusChanged, isListeningForHotkeyChanged, isPickingCoordinateChanged, m_config, m_configUpdateTick, m_countdownDetectionPressedState (+25 more)

### Community 1 - "Community 1"
Cohesion: 0.06
Nodes (33): CountdownConfig, CountdownRuntime, HWND, OverlayConfig, HBITMAP, HDC, ID2D1DCRenderTarget, ID2D1Factory (+25 more)

### Community 2 - "Community 2"
Cohesion: 0.06
Nodes (22): IConfigRepository, IIPCServer, OverlayConfig, unique_ptr, OverlayConfig, QString, IConfigRepository, JsonConfigRepository (+14 more)

### Community 3 - "Community 3"
Cohesion: 0.05
Nodes (37): a, b, g, r, countdown, color, detectionActiveHz, detectionEnabled (+29 more)

### Community 4 - "Community 4"
Cohesion: 0.08
Nodes (20): string, captureScreen(), DetectionRegion, h, w, x, y, ensureCachedTemplate() (+12 more)

### Community 6 - "Community 6"
Cohesion: 0.12
Nodes (15): atomic, HANDLE, jthread, mutex, OverlayConfig, stop_token, IIPCServer, packMessage() (+7 more)

### Community 7 - "Community 7"
Cohesion: 0.12
Nodes (15): ConfigCallback, DisconnectCallback, atomic, HANDLE, jthread, stop_token, IIPCClient, NamedPipeClient (+7 more)

### Community 8 - "Community 8"
Cohesion: 0.11
Nodes (13): HWND, OverlayWindow, m_hwnd, m_paintCallback, m_refreshIntervalMs, m_refreshTimerId, RenderMessage, LPARAM (+5 more)

### Community 11 - "Community 11"
Cohesion: 0.10
Nodes (19): CountdownConfig, color, detectionActiveHz, detectionEnabled, detectionIdleHz, detectionMode, duration, enabled (+11 more)

### Community 12 - "Community 12"
Cohesion: 0.10
Nodes (19): Layer, color, enabled, gap, height, hotkeyModifiers, hotkeyVk, name (+11 more)

### Community 13 - "Community 13"
Cohesion: 0.15
Nodes (15): EngineApp, m_configMutex, m_currentConfig, m_ipc, m_renderer, m_window, IIPCClient, IRenderer (+7 more)

### Community 14 - "Community 14"
Cohesion: 0.17
Nodes (12): ControllerApp, m_config, m_dirty, m_frameCounter, m_ipc, m_repo, m_ui, IConfigRepository (+4 more)

### Community 15 - "Community 15"
Cohesion: 0.32
Nodes (4): OverlayConfig, ImDrawList, ImFont, OverlayUI

### Community 16 - "Community 16"
Cohesion: 0.21
Nodes (14): string, CrosshairPreset, layers, name, posX, posY, uid, from_json() (+6 more)

### Community 17 - "Community 17"
Cohesion: 0.12
Nodes (14): OverlayConfig, countdown, countdownRuntime, editMode, hotkey, hotkeyModifiers, layers, mouseTeleport (+6 more)

### Community 18 - "Community 18"
Cohesion: 0.15
Nodes (12): 1. Backend: `AppBackend`, 1. Detection Pipeline, 2. Frontend: QML UI, 2. State Management & Cooldown, 🏗 Architecture, Assignment Flow, ⌨️ Hotkey Implementation Details, 📡 IPC Protocol (+4 more)

### Community 19 - "Community 19"
Cohesion: 0.22
Nodes (8): 🏗️ Building, Development Requirements (For Developers), 🚀 Features, 📜 License, OverlayX, 📂 Project Structure, 🛠️ Requirements, Runtime Prerequisites (For Users)

### Community 20 - "Community 20"
Cohesion: 0.25
Nodes (6): IIPCClient, connect, disconnect, isConnected, setOnConfigReceived, setOnDisconnected

### Community 21 - "Community 21"
Cohesion: 0.29
Nodes (5): IIPCServer, isClientConnected, sendConfig, start, stop

### Community 22 - "Community 22"
Cohesion: 0.29
Nodes (5): IRenderer, cleanup, initialize, render, resize

### Community 24 - "Community 24"
Cohesion: 0.25
Nodes (7): MouseTeleportConfig, durationMs, enabled, hotkeyModifiers, hotkeyVk, posX, posY

### Community 25 - "Community 25"
Cohesion: 0.60
Nodes (4): string, GetExecutableDirectory(), ResolveAssetPath(), wstring

### Community 26 - "Community 26"
Cohesion: 0.40
Nodes (4): CountdownRuntime, enabled, pausedRemainingMs, startTimestampMs

### Community 27 - "Community 27"
Cohesion: 0.33
Nodes (5): Color, a, b, g, r

### Community 29 - "Community 29"
Cohesion: 0.18
Nodes (10): 1. `controller/` (UI & Configuration Manager), 2. `engine/` (Direct2D Render & Detection Engine), 3. `core/` (Shared Data Entities & Interfaces), AGENTS.md — OverlayX Developer & Agent Guide, 🏛️ Architecture Overview, 🛠️ Build & Development Commands, CMake Build, 📌 Coding & Agent Guidelines (+2 more)

## Knowledge Gaps
- **254 isolated node(s):** `PreToolUse`, `Q_PROPERTY`, `QVariantMap`, `configChanged`, `engineStatusChanged` (+249 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **4 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `AppBackend` connect `Community 0` to `Community 2`, `Community 5`, `Community 9`, `Community 10`, `Community 23`, `Community 28`?**
  _High betweenness centrality (0.299) - this node is a cross-community bridge._
- **Why does `json` connect `Community 16` to `Community 2`, `Community 6`, `Community 7`?**
  _High betweenness centrality (0.291) - this node is a cross-community bridge._
- **What connects `PreToolUse`, `Q_PROPERTY`, `QVariantMap` to the rest of the system?**
  _254 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Community 0` be split into smaller, more focused modules?**
  _Cohesion score 0.03636363636363636 - nodes in this community are weakly interconnected._
- **Should `Community 1` be split into smaller, more focused modules?**
  _Cohesion score 0.056429232192414434 - nodes in this community are weakly interconnected._
- **Should `Community 2` be split into smaller, more focused modules?**
  _Cohesion score 0.06072874493927125 - nodes in this community are weakly interconnected._
- **Should `Community 3` be split into smaller, more focused modules?**
  _Cohesion score 0.05263157894736842 - nodes in this community are weakly interconnected._