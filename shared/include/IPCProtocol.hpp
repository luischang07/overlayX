#pragma once

#include <cstdint>
#include <string>

namespace overlayx {

/// Named pipe path shared between Controller and Engine.
constexpr const wchar_t* PIPE_NAME = L"\\\\.\\pipe\\OverlayXConfig";

/// Wire protocol: [4-byte length][JSON payload]
/// All messages are serialized OverlayConfig as JSON.

inline std::string packMessage(const std::string& json_payload) {
    uint32_t len = static_cast<uint32_t>(json_payload.size());
    std::string msg(sizeof(len) + len, '\0');
    std::memcpy(msg.data(), &len, sizeof(len));
    std::memcpy(msg.data() + sizeof(len), json_payload.data(), len);
    return msg;
}

} // namespace overlayx
