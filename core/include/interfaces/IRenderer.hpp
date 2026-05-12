#pragma once

#include "entities/OverlayConfig.hpp"

namespace overlayx {

/// Interface for rendering shapes on the overlay.
class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual bool initialize(void* windowHandle) = 0;
    virtual void render(const OverlayConfig& config) = 0;
    virtual void resize(int width, int height) = 0;
    virtual void cleanup() = 0;
};

} // namespace overlayx
