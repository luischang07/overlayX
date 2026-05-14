#pragma once
#include "interfaces/IIPCClient.hpp"
#include "interfaces/IRenderer.hpp"
#include "infrastructure/OverlayWindow.hpp"
#include "infrastructure/SpikeDetector.hpp"
#include <memory>
#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>

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

#ifdef SPIKE_DETECTION_ENABLED
      m_spikeDetector.initialize("spike.png");
      m_spikeDetector.setDetectionParams(m_currentConfig.countdown.detectionIdleHz, m_currentConfig.countdown.detectionActiveHz);

      m_stopDetection = false;
      m_detectionThread = std::thread([this]()
                                      {
          while (!m_stopDetection) {
              int delayMs = 100;
              {
                  std::lock_guard lock(m_configMutex);
                  using namespace std::chrono;
                  long long nowMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

                  // Apply detection parameters from latest config
                  m_spikeDetector.setDetectionParams(m_currentConfig.countdown.detectionIdleHz, m_currentConfig.countdown.detectionActiveHz);

                  // Reset spike detected flag after a 45-second cooldown as requested
                  if (m_spikeDetected && (nowMs - m_countdownStartTime >= 45000))
                  {
                      m_spikeDetected = false;
                  }

                  // Manual toggle override: If detection is turned OFF and then ON, clear cooldown
                  if (m_currentConfig.countdown.detectionEnabled && !m_lastDetectionEnabled)
                  {
                      m_spikeDetected = false;
                  }
                  m_lastDetectionEnabled = m_currentConfig.countdown.detectionEnabled;

                  if (m_currentConfig.countdown.detectionEnabled && !m_spikeDetected) {
                      if (m_spikeDetector.detectSpike()) {
                          m_spikeDetected = true;
                          m_countdownStartTime = nowMs;
                          
                          m_currentConfig.countdown.enabled = true;
                          m_currentConfig.countdownRuntime.enabled = true;
                          m_currentConfig.countdownRuntime.startTimestampMs = m_countdownStartTime;
                          m_currentConfig.countdownRuntime.pausedRemainingMs = -1;

                        PostMessage(m_window.handle(), OverlayWindow::RenderMessage, 0, 0);
                      }
                  }

                  // Get adaptive delay
                  delayMs = m_spikeDetector.getSuggestedDelayMs();
              }
              std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
          } });
#endif

      // Keep the overlay idle until a real change arrives.
      m_window.stopRefreshTimer();

      // 3. Set paint callback
      m_window.setPaintCallback([this]()
                                {
            std::lock_guard lock(m_configMutex);

            // Update countdown state before rendering using controller-provided runtime
            if (m_currentConfig.countdownRuntime.enabled && m_currentConfig.countdownRuntime.startTimestampMs > 0) {
                using namespace std::chrono;
              long long nowMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                long long elapsed = nowMs - m_currentConfig.countdownRuntime.startTimestampMs;
                long long durationMs = static_cast<long long>(m_currentConfig.countdown.duration) * 1000LL;
                if (elapsed >= durationMs) {
                    // countdown finished — reflect stopped state locally
                    m_currentConfig.countdownRuntime.enabled = false;
                    m_currentConfig.countdownRuntime.startTimestampMs = 0;
                    m_currentConfig.countdownRuntime.pausedRemainingMs = durationMs;
                }
            }

            if (m_currentConfig.countdown.enabled || m_currentConfig.countdownRuntime.enabled)
            {
              m_window.startRefreshTimer(100);
            }
            else
            {
              m_window.stopRefreshTimer();
            }

            m_renderer->render(m_currentConfig);
            m_window.setEditMode(m_currentConfig.editMode); });

      // 4. Connect IPC — receive config updates
      m_ipc->setOnConfigReceived([this](const OverlayConfig &config)
                                 {
            {
                std::lock_guard lock(m_configMutex);

#ifdef SPIKE_DETECTION_ENABLED
                // State Protection: If we are currently in an active spike-detected countdown,
                // we should preserve our local runtime state unless the incoming config
                // explicitly disables the countdown (manual stop from controller).
                bool localActive = m_spikeDetected && m_currentConfig.countdownRuntime.enabled;
                bool incomingDisable = !config.countdownRuntime.enabled;

                // Detect manual stop/reset coming from controller by comparing incoming config
                // with our previously-held local config. If the controller manually stopped
                // or reset the countdown, clear the spike-detected flag so detection can
                // re-trigger immediately.
                bool incomingManualStop = !config.countdownRuntime.enabled && m_currentConfig.countdownRuntime.enabled;
                bool incomingReset = (config.countdownRuntime.startTimestampMs == 0 && config.countdownRuntime.pausedRemainingMs > 0);

                m_currentConfig = config;

                if (incomingManualStop || incomingReset) {
                    m_spikeDetected = false; // Clear cooldown/flag on manual action
                    m_countdownStartTime = 0;
                }

                if (localActive && !incomingDisable) {
                    // Restore our authoritative local countdown state
                    m_currentConfig.countdownRuntime.enabled = true;
                    m_currentConfig.countdownRuntime.startTimestampMs = m_countdownStartTime;
                    m_currentConfig.countdownRuntime.pausedRemainingMs = -1;
                }
#else
                m_currentConfig = config;
#endif
            }
          PostMessage(m_window.handle(), OverlayWindow::RenderMessage, 0, 0);

            // Handle exit signal
            if (config.shouldExit) {
                PostMessage(m_window.handle(), WM_CLOSE, 0, 0);
                return;
            } });

      m_ipc->setOnDisconnected([this]()
                               {
          // If controller disconnects, exit engine too
          PostMessage(m_window.handle(), WM_CLOSE, 0, 0); });

      m_ipc->connect();

      // 5. Initial render with defaults
      m_renderer->render(m_currentConfig);
      m_window.stopRefreshTimer();

      // 6. Run message loop (blocks until WM_QUIT)
      int result = m_window.runMessageLoop();

      // Cleanup
#ifdef SPIKE_DETECTION_ENABLED
      m_stopDetection = true;
      if (m_detectionThread.joinable())
      {
        m_detectionThread.join();
      }
#endif

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

#ifdef SPIKE_DETECTION_ENABLED
    SpikeDetector m_spikeDetector;
    std::atomic<bool> m_spikeDetected{false};
    std::atomic<long long> m_countdownStartTime{0};
    std::thread m_detectionThread;
    std::atomic<bool> m_stopDetection{false};
    std::atomic<bool> m_lastDetectionEnabled{false};
#endif
  };

} // namespace overlayx
