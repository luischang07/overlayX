#include "app/EngineApp.hpp"
#include "infrastructure/NamedPipeClient.hpp"
#include "rendering/Direct2DRenderer.hpp"

#include <memory>
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Enable DPI awareness to get actual physical resolution
    BOOL dpiResult = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    if (!dpiResult) {
        // Fallback for older Windows versions
        SetProcessDPIAware();
    }

    auto ipc      = std::make_unique<overlayx::NamedPipeClient>();
    auto renderer = std::make_unique<overlayx::Direct2DRenderer>();

    overlayx::EngineApp app(std::move(ipc), std::move(renderer));
    return app.run();
}
