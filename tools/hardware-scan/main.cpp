#include "../../src/hardware/WindowsHardwareProbe.h"
#include "../../src/hardware/InventorySerializer.h"

#include <QCoreApplication>
#include <QFile>
#include <QSaveFile>
#include <QTextStream>

using namespace Orbit::Hardware;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("ORBIT Hardware Scan"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.2.0"));

    if (!WindowsHardwareProbe::isAvailable()) {
        QTextStream(stderr) << "This hardware scan tool currently supports Windows only."
                            << Qt::endl;
        return 2;
    }

    QStringList warnings;
    const HardwareInventorySnapshot snapshot = WindowsHardwareProbe::collect(&warnings);
    const QStringList validationErrors = snapshot.validate();
    if (!validationErrors.isEmpty()) {
        QTextStream errorStream(stderr);
        errorStream << "Inventory validation failed:" << Qt::endl;
        for (const QString &error : validationErrors)
            errorStream << " - " << error << Qt::endl;
        return 3;
    }

    const QByteArray json = InventorySerializer::toJson(snapshot, QJsonDocument::Indented);

    if (argc >= 2) {
        const QString outputPath = QString::fromLocal8Bit(argv[1]);
        QSaveFile output(outputPath);
        if (!output.open(QIODevice::WriteOnly)) {
            QTextStream(stderr) << "Could not open report path: " << outputPath
                                << Qt::endl;
            return 4;
        }
        if (output.write(json) != json.size() || !output.commit()) {
            QTextStream(stderr) << "Could not write the hardware report."
                                << Qt::endl;
            return 5;
        }
        QTextStream(stdout) << "Hardware inventory saved to: " << outputPath << Qt::endl;
    } else {
        QFile output;
        if (!output.open(stdout, QIODevice::WriteOnly))
            return 6;
        output.write(json);
        output.write("\n");
        output.flush();
    }

    if (!warnings.isEmpty()) {
        QTextStream(stderr) << "Scan completed with " << warnings.size()
                            << " warning(s)." << Qt::endl;
    }

    return 0;
}
