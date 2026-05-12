#pragma once

#include "interfaces/IIPCClient.hpp"
#include "IPCProtocol.hpp"
#include <windows.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <atomic>
#include <string>
#include <vector>

namespace overlayx {

/// Named Pipe client: Engine reads config updates from Controller.
class NamedPipeClient final : public IIPCClient {
public:
    ~NamedPipeClient() override { disconnect(); }

    bool connect() override {
        m_running = true;
        m_readThread = std::jthread([this](std::stop_token st) { readLoop(st); });
        return true;
    }

    void disconnect() override {
        m_running = false;
        if (m_readThread.joinable()) {
            m_readThread.request_stop();
        }
        closePipe();
        if (m_readThread.joinable()) {
            m_readThread.join();
        }
    }

    void setOnConfigReceived(ConfigCallback callback) override {
        m_callback = std::move(callback);
    }

    [[nodiscard]] bool isConnected() const override {
        return m_connected;
    }

private:
    void readLoop(std::stop_token st) {
        while (!st.stop_requested() && m_running) {
            // Try to open the pipe
            if (m_hPipe == INVALID_HANDLE_VALUE) {
                m_hPipe = CreateFileW(
                    PIPE_NAME,
                    GENERIC_READ,
                    0, nullptr,
                    OPEN_EXISTING,
                    0, nullptr
                );

                if (m_hPipe == INVALID_HANDLE_VALUE) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    continue;
                }
                m_connected = true;
            }

            // Read length-prefixed message
            uint32_t msgLen = 0;
            DWORD bytesRead = 0;
            BOOL ok = ReadFile(m_hPipe, &msgLen, sizeof(msgLen), &bytesRead, nullptr);

            if (!ok || bytesRead != sizeof(msgLen)) {
                // Pipe broken — Controller likely exited
                closePipe();
                m_connected = false;
                if (m_running) {
                    // Try to reconnect
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
                continue;
            }

            // Read JSON payload
            std::vector<char> buffer(msgLen);
            ok = ReadFile(m_hPipe, buffer.data(), msgLen, &bytesRead, nullptr);

            if (!ok || bytesRead != msgLen) {
                closePipe();
                m_connected = false;
                continue;
            }

            // Parse and dispatch
            try {
                std::string json_str(buffer.data(), buffer.size());
                nlohmann::json j = nlohmann::json::parse(json_str);
                OverlayConfig config = j.get<OverlayConfig>();
                if (m_callback) {
                    m_callback(config);
                }
            } catch (...) {
                // Malformed message — skip
            }
        }
    }

    void closePipe() {
        if (m_hPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(m_hPipe);
            m_hPipe = INVALID_HANDLE_VALUE;
        }
    }

    HANDLE              m_hPipe{INVALID_HANDLE_VALUE};
    std::atomic<bool>   m_running{false};
    std::atomic<bool>   m_connected{false};
    ConfigCallback      m_callback;
    std::jthread        m_readThread;
};

} // namespace overlayx
