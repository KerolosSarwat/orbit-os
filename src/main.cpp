#include "AppController.h"
#include "SystemMonitor.h"
#include "hardware/HardwareInspectorService.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCoreApplication::setApplicationName(QStringLiteral("ORBIT Shell"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.4.0"));
    QCoreApplication::setOrganizationName(QStringLiteral("ORBIT Project"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("orbit.local"));

    AppController orbit;
    SystemMonitor systemMonitor;
    Orbit::Hardware::HardwareInspectorService hardwareInspector;
    systemMonitor.setRefreshIntervalMs(orbit.metricsRefreshIntervalMs());

    QObject::connect(&orbit, &AppController::metricsRefreshIntervalMsChanged,
                     [&orbit, &systemMonitor]() {
                         systemMonitor.setRefreshIntervalMs(orbit.metricsRefreshIntervalMs());
                     });

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("orbit"), &orbit);
    engine.rootContext()->setContextProperty(QStringLiteral("systemMonitor"), &systemMonitor);
    engine.rootContext()->setContextProperty(QStringLiteral("hardwareInspector"), &hardwareInspector);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        [] { QCoreApplication::exit(EXIT_FAILURE); },
        Qt::QueuedConnection);

    engine.loadFromModule(QStringLiteral("Orbit.Shell"), QStringLiteral("Main"));

    return app.exec();
}
