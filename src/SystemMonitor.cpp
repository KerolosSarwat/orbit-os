#include <QtGlobal>

#ifdef Q_OS_WIN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "SystemMonitor.h"

#include <QDir>
#include <QFile>
#include <QStorageInfo>
#include <QStringList>

SystemMonitor::SystemMonitor(QObject *parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &SystemMonitor::refreshNow);
    m_timer.setInterval(2000);
    refreshNow();
    m_timer.start();
}

int SystemMonitor::cpuPercent() const { return m_cpuPercent; }
qulonglong SystemMonitor::memoryUsedBytes() const { return m_memoryUsedBytes; }
qulonglong SystemMonitor::memoryTotalBytes() const { return m_memoryTotalBytes; }
int SystemMonitor::memoryPercent() const { return m_memoryPercent; }
qulonglong SystemMonitor::storageUsedBytes() const { return m_storageUsedBytes; }
qulonglong SystemMonitor::storageTotalBytes() const { return m_storageTotalBytes; }
qulonglong SystemMonitor::storageAvailableBytes() const { return m_storageAvailableBytes; }
int SystemMonitor::storagePercent() const { return m_storagePercent; }
QString SystemMonitor::storageRoot() const { return m_storageRoot; }

QString SystemMonitor::formatBytes(qulonglong bytes) const
{
    static const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    double value = static_cast<double>(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        ++unit;
    }
    const int precision = unit == 0 ? 0 : (value >= 100.0 ? 0 : 1);
    return QStringLiteral("%1 %2").arg(QString::number(value, 'f', precision),
                                       QString::fromLatin1(units[unit]));
}

void SystemMonitor::setRefreshIntervalMs(int intervalMs)
{
    if (intervalMs != 1000 && intervalMs != 2000 && intervalMs != 5000)
        return;
    m_timer.setInterval(intervalMs);
}

void SystemMonitor::refreshNow()
{
#ifdef Q_OS_WIN
    FILETIME idleTime{};
    FILETIME kernelTime{};
    FILETIME userTime{};
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        ULARGE_INTEGER idleValue{};
        ULARGE_INTEGER kernelValue{};
        ULARGE_INTEGER userValue{};
        idleValue.LowPart = idleTime.dwLowDateTime;
        idleValue.HighPart = idleTime.dwHighDateTime;
        kernelValue.LowPart = kernelTime.dwLowDateTime;
        kernelValue.HighPart = kernelTime.dwHighDateTime;
        userValue.LowPart = userTime.dwLowDateTime;
        userValue.HighPart = userTime.dwHighDateTime;

        const quint64 idle = idleValue.QuadPart;
        const quint64 kernel = kernelValue.QuadPart;
        const quint64 user = userValue.QuadPart;
        const quint64 total = kernel + user;

        if (m_hasCpuSample && total > (m_previousKernel + m_previousUser)) {
            const quint64 totalDelta = total - (m_previousKernel + m_previousUser);
            const quint64 idleDelta = idle - m_previousIdle;
            const double busy = totalDelta > 0
                ? 100.0 * (1.0 - static_cast<double>(idleDelta) / static_cast<double>(totalDelta))
                : 0.0;
            m_cpuPercent = qBound(0, qRound(busy), 100);
        }
        m_previousIdle = idle;
        m_previousKernel = kernel;
        m_previousUser = user;
        m_hasCpuSample = true;
    }

    MEMORYSTATUSEX memoryStatus{};
    memoryStatus.dwLength = sizeof(memoryStatus);
    if (GlobalMemoryStatusEx(&memoryStatus)) {
        m_memoryTotalBytes = memoryStatus.ullTotalPhys;
        m_memoryUsedBytes = memoryStatus.ullTotalPhys - memoryStatus.ullAvailPhys;
        m_memoryPercent = m_memoryTotalBytes > 0
            ? qBound(0, qRound(100.0 * static_cast<double>(m_memoryUsedBytes)
                               / static_cast<double>(m_memoryTotalBytes)), 100)
            : 0;
    }
#else
    QFile cpuFile(QStringLiteral("/proc/stat"));
    if (cpuFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString line = QString::fromUtf8(cpuFile.readLine()).simplified();
        const QStringList fields = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (fields.size() >= 5 && fields.first() == QStringLiteral("cpu")) {
            quint64 total = 0;
            // /proc/stat's guest and guest_nice counters are already included in
            // user and nice, so exclude them to avoid double-counting CPU time.
            for (qsizetype i = 1; i < fields.size() && i < 9; ++i) {
                bool ok = false;
                const quint64 value = fields.at(i).toULongLong(&ok);
                if (ok)
                    total += value;
            }

            const quint64 idle = fields.at(4).toULongLong()
                + (fields.size() > 5 ? fields.at(5).toULongLong() : 0);
            if (m_hasCpuSample && total > m_previousTotalTicks) {
                const quint64 totalDelta = total - m_previousTotalTicks;
                const quint64 idleDelta = idle - m_previousIdleTicks;
                const double busy = totalDelta > 0
                    ? 100.0 * (1.0 - static_cast<double>(idleDelta) / static_cast<double>(totalDelta))
                    : 0.0;
                m_cpuPercent = qBound(0, qRound(busy), 100);
            }
            m_previousTotalTicks = total;
            m_previousIdleTicks = idle;
            m_hasCpuSample = true;
        }
    }

    QFile memoryFile(QStringLiteral("/proc/meminfo"));
    if (memoryFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        quint64 totalKb = 0;
        quint64 availableKb = 0;
        while (!memoryFile.atEnd()) {
            const QStringList fields = QString::fromUtf8(memoryFile.readLine())
                                           .simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
            if (fields.size() < 2)
                continue;
            if (fields.at(0) == QStringLiteral("MemTotal:"))
                totalKb = fields.at(1).toULongLong();
            else if (fields.at(0) == QStringLiteral("MemAvailable:"))
                availableKb = fields.at(1).toULongLong();
        }
        if (totalKb > 0) {
            m_memoryTotalBytes = totalKb * 1024ULL;
            const quint64 availableBytes = qMin(availableKb * 1024ULL, m_memoryTotalBytes);
            m_memoryUsedBytes = m_memoryTotalBytes - availableBytes;
            m_memoryPercent = qBound(0, qRound(100.0 * static_cast<double>(m_memoryUsedBytes)
                                                / static_cast<double>(m_memoryTotalBytes)), 100);
        }
    }
#endif

    QStorageInfo storage(QDir::rootPath());
    storage.refresh();
    if (storage.isValid() && storage.isReady() && storage.bytesTotal() > 0) {
        m_storageRoot = storage.rootPath();
        m_storageTotalBytes = static_cast<qulonglong>(storage.bytesTotal());
        m_storageAvailableBytes = static_cast<qulonglong>(qMax<qint64>(0, storage.bytesAvailable()));
        const qint64 freeBytes = qMax<qint64>(0, storage.bytesFree());
        m_storageUsedBytes = m_storageTotalBytes > static_cast<qulonglong>(freeBytes)
            ? m_storageTotalBytes - static_cast<qulonglong>(freeBytes)
            : 0;
        m_storagePercent = qBound(0, qRound(100.0 * static_cast<double>(m_storageUsedBytes)
                                            / static_cast<double>(m_storageTotalBytes)), 100);
    }

    emit metricsChanged();
}
