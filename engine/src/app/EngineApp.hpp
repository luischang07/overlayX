#pragma once

#include "entities/OverlayConfig.hpp"
#include "interfaces/IIPCClient.hpp"
#include "interfaces/IRenderer.hpp"
#include "infrastructure/OverlayWindow.hpp"
#include <memory>
#include <mutex>
#include <atomic>
#include <chrono>

namespace overlayx
{

  /// Engine orchestrator: wires Window, Renderer, and IPC client.
  class EngineApp
  {
  public:
    EngineApp(std::unique_ptr<IIPCClient> ipc,
              std::unique_ptr<IRenderer> renderer)
        : m_ipc(std::move(ipc)), m_renderer(std::move(renderer))
    {
    }

    int run()
    {
      // 1. Create overlay window
      if (!m_window.create())
        return 1;

      // 2. Initialize renderer
      if (!m_renderer->initialize(m_window.handle()))
        return 1;

      // Drive periodic repaints so the countdown updates visually while running.
      m_window.startRefreshTimer(33);

      // 3. Set paint callback
      m_window.setPaintCallback([this]()
                                {
            std::lock_guard lock(m_configMutex);
            // Update countdown state before rendering using controller-provided runtime
            if (m_currentConfig.countdownRuntime.enabled && m_currentConfig.countdownRuntime.startTimestampMs > 0) {
                using namespace std::chrono;
                long long nowMs = duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
                long long elapsed = nowMs - m_currentConfig.countdownRuntime.startTimestampMs;
                long long durationMs = static_cast<long long>(m_currentConfig.countdown.duration) * 1000LL;
                if (elapsed >= durationMs) {
                    // countdown finished — reflect stopped state locally
                    m_currentConfig.countdownRuntime.enabled = false;
                    m_currentConfig.countdownRuntime.startTimestampMs = 0;
                    m_currentConfig.countdownRuntime.pausedRemainingMs = durationMs;
                }
            }
            m_renderer->render(m_currentConfig);
            m_window.setEditMode(m_currentConfig.editMode); });

      // 4. Connect IPC — receive config updates
      m_ipc->setOnConfigReceived([this](const OverlayConfig &config)
                                 {
            {
                std::lock_guard lock(m_configMutex);
                m_currentConfig = config;
            }
            // Trigger repaint on the main thread
            m_window.invalidate();

            // Handle exit signal
            if (config.shouldExit) {
                PostQuitMessage(0);
                return;
            } });

      m_ipc->connect();

      // 5. Initial render with defaults
      m_renderer->render(m_currentConfig);

      // 6. Run message loop (blocks until WM_QUIT)
      int result = m_window.runMessageLoop();

      // Cleanup
      m_window.stopRefreshTimer();
      m_ipc->disconnect();
      m_renderer->cleanup();
      return result;
    }

    // Engine relies on controller-provided CountdownRuntime; no local control methods needed

  private:
    OverlayWindow m_window;
    std::unique_ptr<IIPCClient> m_ipc;
    std::unique_ptr<IRenderer> m_renderer;
    OverlayConfig m_currentConfig;
    std::mutex m_configMutex;
  };

} // namespace overlayx
