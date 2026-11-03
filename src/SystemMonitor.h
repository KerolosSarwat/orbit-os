#pragma once

#include <QObject>
#include <QTimer>
#include <QString>

class SystemMonitor final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int cpuPercent READ cpuPercent NOTIFY metricsChanged)
    Q_PROPERTY(qulonglong memoryUsedBytes READ memoryUsedBytes NOTIFY metricsChanged)
    Q_PROPERTY(qulonglong memoryTotalBytes READ memoryTotalBytes NOTIFY metricsChanged)
    Q_PROPERTY(int memoryPercent READ memoryPercent NOTIFY metricsChanged)
    Q_PROPERTY(qulonglong storageUsedBytes READ storageUsedBytes NOTIFY metricsChanged)
    Q_PROPERTY(qulonglong storageTotalBytes READ storageTotalBytes NOTIFY metricsChanged)
    Q_PROPERTY(qulonglong storageAvailableBytes READ storageAvailableBytes NOTIFY metricsChanged)
    Q_PROPERTY(int storagePercent READ storagePercent NOTIFY metricsChanged)
    Q_PROPERTY(QString storageRoot READ storageRoot NOTIFY metricsChanged)

public:
    explicit SystemMonitor(QObject *parent = nullptr);

    int cpuPercent() const;
    qulonglong memoryUsedBytes() const;
    qulonglong memoryTotalBytes() const;
    int memoryPercent() const;
    qulonglong storageUsedBytes() const;
    qulonglong storageTotalBytes() const;
    qulonglong storageAvailableBytes() const;
    int storagePercent() const;
    QString storageRoot() const;

    Q_INVOKABLE QString formatBytes(qulonglong bytes) const;

public slots:
    void setRefreshIntervalMs(int intervalMs);
    void refreshNow();

signals:
    void metricsChanged();

private:
    QTimer m_timer;
    int m_cpuPercent = -1;
    qulonglong m_memoryUsedBytes = 0;
    qulonglong m_memoryTotalBytes = 0;
    int m_memoryPercent = 0;
    qulonglong m_storageUsedBytes = 0;
    qulonglong m_storageTotalBytes = 0;
    qulonglong m_storageAvailableBytes = 0;
    int m_storagePercent = 0;
    QString m_storageRoot;

#ifdef Q_OS_WIN
    quint64 m_previousIdle = 0;
    quint64 m_previousKernel = 0;
    quint64 m_previousUser = 0;
    bool m_hasCpuSample = false;
#else
    quint64 m_previousTotalTicks = 0;
    quint64 m_previousIdleTicks = 0;
    bool m_hasCpuSample = false;
#endif
};
