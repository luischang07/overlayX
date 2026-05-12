#pragma once

#include "interfaces/IIPCServer.hpp"
#include "IPCProtocol.hpp"
#include <windows.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <atomic>
#include <mutex>

namespace overlayx {

/// Named Pipe server: Controller writes config updates to the Engine.
class NamedPipeServer final : public IIPCServer {
public:
    ~NamedPipeServer() override { stop(); }

    bool start() override {
        m_running = true;
        m_listenThread = std::jthread([this](std::stop_token st) { listenLoop(st); });
        return true;
    }

    void stop() override {
        m_running = false;
        if (m_listenThread.joinable()) {
            m_listenThread.request_stop();
            
            // Interrupt blocking ConnectNamedPipe by closing/canceling
            {
                std::lock_guard lock(m_pipeMutex);
                if (m_hPipe != INVALID_HANDLE_VALUE) {
                    CancelIoEx(m_hPipe, nullptr);
                    closePipe();
                }
            }
            
            m_listenThread.join();
        }
    }

    bool sendConfig(const OverlayConfig& config) override {
        std::lock_guard lock(m_pipeMutex);
        if (m_hPipe == INVALID_HANDLE_VALUE || !m_clientConnected) return false;

        try {
            nlohmann::json j = config;
            std::string payload = j.dump();
            std::string msg = packMessage(payload);

            DWORD written = 0;
            BOOL ok = WriteFile(m_hPipe, msg.data(),
                                static_cast<DWORD>(msg.size()), &written, nullptr);
            if (!ok) {
                // Client disconnected
                m_clientConnected = false;
                closePipe();
                return false;
            }
            return true;
        } catch (...) {
            return false;
        }
    }

    [[nodiscard]] bool isClientConnected() const override {
        return m_clientConnected;
    }

private:
    void listenLoop(std::stop_token st) {
        while (!st.stop_requested() && m_running) {
            {
                std::lock_guard lock(m_pipeMutex);
                if (m_hPipe == INVALID_HANDLE_VALUE) {
                    m_hPipe = CreateNamedPipeW(
                        PIPE_NAME,
                        PIPE_ACCESS_OUTBOUND,
                        PIPE_TYPE_BYTE | PIPE_WAIT,
                        1,        // max instances
                        65536,    // out buffer
                        0,        // in buffer
                        0,        // default timeout
                        nullptr
                    );
                }
            }

            if (m_hPipe == INVALID_HANDLE_VALUE) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }

            // Blocking wait for a client connection
            BOOL connected = ConnectNamedPipe(m_hPipe, nullptr);
            if (connected || GetLastError() == ERROR_PIPE_CONNECTED) {
                m_clientConnected = true;
                // Stay connected until pipe breaks or stop requested
                while (!st.stop_requested() && m_clientConnected && m_running) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }

            // Reset for next connection
            std::lock_guard lock(m_pipeMutex);
            closePipe();
            m_clientConnected = false;
        }
    }

    void closePipe() {
        if (m_hPipe != INVALID_HANDLE_VALUE) {
            DisconnectNamedPipe(m_hPipe);
            CloseHandle(m_hPipe);
            m_hPipe = INVALID_HANDLE_VALUE;
        }
    }

    HANDLE               m_hPipe{INVALID_HANDLE_VALUE};
    std::atomic<bool>    m_running{false};
    std::atomic<bool>    m_clientConnected{false};
    std::jthread         m_listenThread;
    std::mutex           m_pipeMutex;
};

} // namespace overlayx
