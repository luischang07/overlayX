#pragma once

#include "entities/OverlayConfig.hpp"
#include <functional>

namespace overlayx {

/// Interface for IPC receiving (Engine side).
class IIPCClient {
public:
    using ConfigCallback = std::function<void(const OverlayConfig&)>;
    using DisconnectCallback = std::function<void()>;

    virtual ~IIPCClient() = default;
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual void setOnConfigReceived(ConfigCallback callback) = 0;
    virtual void setOnDisconnected(DisconnectCallback callback) = 0;
    [[nodiscard]] virtual bool isConnected() const = 0;
};

} // namespace overlayx
