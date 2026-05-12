#pragma once

#include "interfaces/IConfigRepository.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

namespace overlayx {

/// Concrete persistence: saves/loads OverlayConfig to a JSON file.
class JsonConfigRepository final : public IConfigRepository {
public:
    explicit JsonConfigRepository(std::filesystem::path path)
        : m_path(std::move(path)) {}

    bool save(const OverlayConfig& config) override {
        try {
            nlohmann::json j = config;
            std::ofstream ofs(m_path);
            if (!ofs.is_open()) return false;
            ofs << j.dump(4);
            return true;
        } catch (...) {
            return false;
        }
    }

    std::optional<OverlayConfig> load() override {
        try {
            if (!std::filesystem::exists(m_path)) return std::nullopt;
            std::ifstream ifs(m_path);
            if (!ifs.is_open()) return std::nullopt;
            nlohmann::json j;
            ifs >> j;
            return j.get<OverlayConfig>();
        } catch (...) {
            return std::nullopt;
        }
    }

private:
    std::filesystem::path m_path;
};

} // namespace overlayx
