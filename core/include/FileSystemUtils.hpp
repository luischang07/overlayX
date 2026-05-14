#pragma once

#include <windows.h>
#include <string>
#include <filesystem>

namespace overlayx {
namespace fs {

/**
 * @brief Retrieves the absolute path to the directory containing the current executable.
 */
inline std::wstring GetExecutableDirectory() {
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    std::filesystem::path fullPath(buffer);
    return fullPath.parent_path().wstring();
}

/**
 * @brief Resolves a path relative to the 'assets' subdirectory in the executable's folder.
 * @param filename The name of the file or relative path within assets/
 */
inline std::wstring ResolveAssetPath(const std::wstring& filename) {
    std::filesystem::path exeDir(GetExecutableDirectory());
    return (exeDir / L"assets" / filename).wstring();
}

/**
 * @brief Overload for std::string (converts to wstring internally for Windows API compatibility)
 */
inline std::string ResolveAssetPath(const std::string& filename) {
    std::filesystem::path exeDir(GetExecutableDirectory());
    return (exeDir / "assets" / filename).string();
}

} // namespace fs
} // namespace overlayx
