#include "HardwareInspectorService.h"

#include "InventorySerializer.h"
#include "WindowsHardwareProbe.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFutureWatcher>
#include <QStandardPaths>
#include <QSaveFile>
#include <QtConcurrent/QtConcurrentRun>

namespace Orbit::Hardware {
namespace {

struct ScanResult
{
    HardwareInventorySnapshot snapshot;
    QStringList errors;
};

ScanResult runHardwareScan()
{
    ScanResult result;
    QStringList warnings;
    result.snapshot = WindowsHardwareProbe::collect(&warnings);
    result.errors = result.snapshot.validate();
    return result;
}

} // namespace

HardwareInspectorService::HardwareInspectorService(QObject *parent)
    : QObject(parent),
      m_available(WindowsHardwareProbe::isAvailable())
{
    m_statusMessage = m_available
        ? QStringLiteral("No hardware scan has been run in this session.")
        : QStringLiteral("Live hardware discovery is currently available on Windows only.");
}

bool HardwareInspectorService::available() const
{
    return m_available;
}

bool HardwareInspectorService::scanning() const
{
    return m_scanning;
}

bool HardwareInspectorService::hasInventory() const
{
    return m_hasInventory;
}

QVariantMap HardwareInspectorService::inventory() const
{
    return m_inventory;
}

QString HardwareInspectorService::statusMessage() const
{
    return m_statusMessage;
}

QString HardwareInspectorService::lastScanAt() const
{
    return m_lastScanAt;
}

void HardwareInspectorService::scan()
{
    if (!m_available) {
        m_statusMessage = QStringLiteral(
            "This build does not yet include a hardware collector for the current operating system.");
        emit statusMessageChanged();
        return;
    }

    if (m_scanning)
        return;

    m_scanning = true;
    m_statusMessage = QStringLiteral("Scanning local hardware using read-only system APIs…");
    emit scanningChanged();
    emit statusMessageChanged();

    auto *watcher = new QFutureWatcher<ScanResult>(this);
    connect(watcher, &QFutureWatcher<ScanResult>::finished, this, [this, watcher]() {
        const ScanResult result = watcher->result();
        watcher->deleteLater();

        m_scanning = false;
        emit scanningChanged();

        if (!result.errors.isEmpty()) {
            m_statusMessage = QStringLiteral("The inventory report did not pass validation: %1")
                                  .arg(result.errors.join(QStringLiteral("; ")));
            emit statusMessageChanged();
            return;
        }

        m_latestSnapshot = result.snapshot;
        m_latestJson = InventorySerializer::toJson(m_latestSnapshot);
        m_inventory = m_latestSnapshot.toJson().toVariantMap();
        m_hasInventory = true;
        m_lastScanAt = m_latestSnapshot.collectedAtUtc.toLocalTime()
                           .toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
        m_statusMessage = QStringLiteral("Hardware scan completed. Inventory evidence is ready to inspect.");
        emit inventoryChanged();
        emit statusMessageChanged();
    });

    watcher->setFuture(QtConcurrent::run(&runHardwareScan));
}

QString HardwareInspectorService::exportReport()
{
    if (!m_hasInventory || m_latestJson.isEmpty())
        return QStringLiteral("Run a successful hardware scan before exporting a report.");

    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (documentsPath.isEmpty())
        documentsPath = QDir::homePath();

    QDir documents(documentsPath);
    const QString reportDirectory = QStringLiteral("ORBIT/HardwareReports");
    if (!documents.mkpath(reportDirectory))
        return QStringLiteral("Could not create the ORBIT HardwareReports folder.");

    const QString timestamp = m_latestSnapshot.collectedAtUtc.toString(
        QStringLiteral("yyyyMMdd-HHmmss-zzz"));
    const QString reportPath = documents.filePath(
        reportDirectory + QStringLiteral("/hardware-report-") + timestamp + QStringLiteral(".json"));

    QSaveFile report(reportPath);
    if (!report.open(QIODevice::WriteOnly))
        return QStringLiteral("Could not write the hardware report: %1").arg(report.errorString());

    if (report.write(m_latestJson) != m_latestJson.size()) {
        const QString error = report.errorString();
        report.cancelWriting();
        return QStringLiteral("The hardware report could not be written completely: %1").arg(error);
    }

    if (!report.commit())
        return QStringLiteral("Could not finalize the hardware report: %1").arg(report.errorString());
    m_statusMessage = QStringLiteral("Report exported to %1").arg(reportPath);
    emit statusMessageChanged();
    return QStringLiteral("Hardware report saved: %1").arg(reportPath);
}

} // namespace Orbit::Hardware
