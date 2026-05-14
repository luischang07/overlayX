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

#include <QFile>
#include <QTextStream>
#include <QDateTime>

void logToFile(const QString &message) {
    // Disabled for production
    /*
    QFile file("controller_log.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&file);
        stream << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz ") << message << "\n";
    }
    */
}

int main(int argc, char* argv[]) {
    logToFile("Starting OverlayX Controller...");
    
    QGuiApplication app(argc, argv);
    
    // Add application directory to import path for portable QML modules
    QQmlApplicationEngine engine;
    engine.addImportPath(app.applicationDirPath() + "/qml");
    logToFile("Import paths: " + engine.importPathList().join(", "));
    
    // Create dependencies
    auto repo = std::make_unique<JsonConfigRepository>("overlayx_config.json");
    auto pipe = std::make_unique<NamedPipeServer>();
    
    // Create and register backend
    AppBackend backend(std::move(repo), std::move(pipe));
    
    // Catch QML errors
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, [](const QList<QQmlError> &warnings) {
        for (const auto &w : warnings) {
            logToFile("QML Warning/Error: " + w.toString());
        }
    });

    engine.rootContext()->setContextProperty("backend", &backend);
    
    logToFile("Loading module OverlayX.Main...");
    engine.loadFromModule("OverlayX", "Main");
    
    if (engine.rootObjects().isEmpty()) {
        logToFile("ERROR: No root objects loaded!");
        return -1;
    }

    logToFile("App running.");
    return app.exec();
}


