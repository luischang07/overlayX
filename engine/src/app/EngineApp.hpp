#pragma once

#include "entities/OverlayConfig.hpp"
#include "interfaces/IIPCClient.hpp"
#include "interfaces/IRenderer.hpp"
#include "infrastructure/OverlayWindow.hpp"
#include <memory>
#include <mutex>
#include <atomic>

namespace overlayx {

/// Engine orchestrator: wires Window, Renderer, and IPC client.
class EngineApp {
public:
    EngineApp(std::unique_ptr<IIPCClient> ipc,
              std::unique_ptr<IRenderer> renderer)
        : m_ipc(std::move(ipc))
        , m_renderer(std::move(renderer))
    {}

    int run() {
        // 1. Create overlay window
        if (!m_window.create()) return 1;

        // 2. Initialize renderer
        if (!m_renderer->initialize(m_window.handle())) return 1;

        // 3. Set paint callback
        m_window.setPaintCallback([this]() {
            std::lock_guard lock(m_configMutex);
            m_renderer->render(m_currentConfig);
            m_window.setEditMode(m_currentConfig.editMode);
        });

        // 4. Connect IPC — receive config updates
        m_ipc->setOnConfigReceived([this](const OverlayConfig& config) {
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
            }
        });

        m_ipc->connect();

        // 5. Initial render with defaults
        m_renderer->render(m_currentConfig);

        // 6. Run message loop (blocks until WM_QUIT)
        int result = m_window.runMessageLoop();

        // Cleanup
        m_ipc->disconnect();
        m_renderer->cleanup();
        return result;
    }

private:
    OverlayWindow                m_window;
    std::unique_ptr<IIPCClient>  m_ipc;
    std::unique_ptr<IRenderer>   m_renderer;
    OverlayConfig                m_currentConfig;
    std::mutex                   m_configMutex;
};

} // namespace overlayx
