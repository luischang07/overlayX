#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <iostream>
#include <memory>
#include "app/AppBackend.hpp"
#include "infrastructure/JsonConfigRepository.hpp"
#include "infrastructure/NamedPipeServer.hpp"

using namespace overlayx;

int main(int argc, char* argv[]) {
    std::cout << "Starting OverlayX Controller..." << std::endl;
    
    QGuiApplication app(argc, argv);
    
    // Create dependencies
    auto repo = std::make_unique<JsonConfigRepository>("overlayx_config.json");
    auto pipe = std::make_unique<NamedPipeServer>();
    
    // Create and register backend
    AppBackend backend(std::move(repo), std::move(pipe));
    
    QQmlApplicationEngine engine;
    
    // Catch QML errors
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, [](const QList<QQmlError> &warnings) {
        for (const auto &w : warnings) {
            std::cerr << "QML Warning/Error: " << w.toString().toStdString() << std::endl;
        }
    });

    engine.rootContext()->setContextProperty("backend", &backend);
    
    engine.loadFromModule("OverlayX", "Main");

    
    if (engine.rootObjects().isEmpty()) {
        std::cerr << "No root objects loaded!" << std::endl;
        return -1;
    }

    std::cout << "App running." << std::endl;
    return app.exec();
}


