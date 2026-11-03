#pragma once

#include <QObject>
#include <QString>

class AppController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString activeWorkspace READ activeWorkspace NOTIFY activeWorkspaceChanged)
    Q_PROPERTY(bool showSystemMetrics READ showSystemMetrics NOTIFY showSystemMetricsChanged)
    Q_PROPERTY(int metricsRefreshIntervalMs READ metricsRefreshIntervalMs NOTIFY metricsRefreshIntervalMsChanged)

public:
    explicit AppController(QObject *parent = nullptr);

    QString activeWorkspace() const;
    bool showSystemMetrics() const;
    int metricsRefreshIntervalMs() const;

    Q_INVOKABLE QString launchBrowser() const;
    Q_INVOKABLE QString launchTerminal() const;
    Q_INVOKABLE QString folderPath(const QString &folderId) const;
    Q_INVOKABLE QString openFolder(const QString &folderId) const;

    Q_INVOKABLE void setActiveWorkspace(const QString &workspace);
    Q_INVOKABLE void setShowSystemMetrics(bool enabled);
    Q_INVOKABLE void setMetricsRefreshIntervalMs(int intervalMs);

signals:
    void activeWorkspaceChanged();
    void showSystemMetricsChanged();
    void metricsRefreshIntervalMsChanged();

private:
    QString m_activeWorkspace;
    bool m_showSystemMetrics = true;
    int m_metricsRefreshIntervalMs = 2000;
};
