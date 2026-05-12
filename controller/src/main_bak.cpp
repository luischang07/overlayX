#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include <QIcon>
#include <filesystem>
#include <memory>

#include "app/AppBackend.hpp"
#include "infrastructure/JsonConfigRepository.hpp"
#include "infrastructure/NamedPipeServer.hpp"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("OverlayX Controller");
    app.setOrganizationName("OverlayX");

    // ─── Wire up Clean Architecture ────────────────────────
    auto configPath = std::filesystem::current_path() / "overlayx_config.json";
    auto repo = std::make_unique<overlayx::JsonConfigRepository>(configPath);
    auto ipc  = std::make_unique<overlayx::NamedPipeServer>();

    overlayx::AppBackend backend(std::move(repo), std::move(ipc));

    // ─── Load QML ──────────────────────────────────────────
    QQmlApplicationEngine engine;
    
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.rootContext()->setContextProperty("backend", &backend);

    qDebug() << "Loading QML module...";
    engine.loadFromModule("OverlayX", "Main");

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Error: No root objects loaded. QML might have failed to load.";
        return -1;
    }
    
    qDebug() << "Window loaded successfully.";

    return app.exec();
}
