#pragma once

#include "entities/OverlayConfig.hpp"
#include <optional>

namespace overlayx {

/// Interface Segregation: Only persistence concerns.
class IConfigRepository {
public:
    virtual ~IConfigRepository() = default;
    virtual bool save(const OverlayConfig& config) = 0;
    virtual std::optional<OverlayConfig> load() = 0;
};

} // namespace overlayx
