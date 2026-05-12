#pragma once

#include "entities/OverlayConfig.hpp"

namespace overlayx {

/// Interface for IPC broadcasting (Controller -> Engine).
class IIPCServer {
public:
    virtual ~IIPCServer() = default;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool sendConfig(const OverlayConfig& config) = 0;
    [[nodiscard]] virtual bool isClientConnected() const = 0;
};

} // namespace overlayx
