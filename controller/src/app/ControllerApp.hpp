#pragma once

#include "entities/OverlayConfig.hpp"
#include "interfaces/IConfigRepository.hpp"
#include "interfaces/IIPCServer.hpp"
#include "ui/OverlayUI.hpp"
#include <memory>
#include <thread>
#include <chrono>

namespace overlayx {

/// MVC Controller: orchestrates Model (config), View (UI), and IPC.
class ControllerApp {
public:
    ControllerApp(std::unique_ptr<IConfigRepository> repo,
                  std::unique_ptr<IIPCServer> ipc)
        : m_repo(std::move(repo))
        , m_ipc(std::move(ipc))
    {
        // Load persisted config or use defaults
        auto loaded = m_repo->load();
        if (loaded) m_config = *loaded;
    }

    void start() {
        m_ipc->start();
        OverlayUI::applyTheme();
    }

    void stop() {
        m_repo->save(m_config);
        m_config.visible = false;
        m_config.shouldExit = true;  // Signal engine to terminate
        m_ipc->sendConfig(m_config);
        
        // Give a tiny bit of time for the message to be sent
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        m_ipc->stop();
    }

    /// Call every frame from the ImGui render loop.
    void tick() {
        bool modified = m_ui.render(m_config, m_ipc->isClientConnected());
        if (modified) {
            m_ipc->sendConfig(m_config);
            m_dirty = true;
        }
        // Auto-save periodically
        m_frameCounter++;
        if (m_dirty && (m_frameCounter % 300 == 0)) {  // ~5 sec at 60fps
            m_repo->save(m_config);
            m_dirty = false;
        }
    }

private:
    OverlayConfig                   m_config;
    OverlayUI                       m_ui;
    std::unique_ptr<IConfigRepository> m_repo;
    std::unique_ptr<IIPCServer>     m_ipc;
    bool                            m_dirty{false};
    uint64_t                        m_frameCounter{0};
};

} // namespace overlayx
