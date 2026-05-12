#include "app/EngineApp.hpp"
#include "infrastructure/NamedPipeClient.hpp"
#include "rendering/Direct2DRenderer.hpp"

#include <memory>

int main() {
    auto ipc      = std::make_unique<overlayx::NamedPipeClient>();
    auto renderer = std::make_unique<overlayx::Direct2DRenderer>();

    overlayx::EngineApp app(std::move(ipc), std::move(renderer));
    return app.run();
}
