#include "AppController.h"

#include <QDesktopServices>
#include <QDir>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QUrl>

AppController::AppController(QObject *parent)
    : QObject(parent)
{
    QSettings settings;
    m_activeWorkspace = settings.value(QStringLiteral("workspace/active"),
                                        QStringLiteral("Focus")).toString();
    if (m_activeWorkspace != QStringLiteral("Focus")
        && m_activeWorkspace != QStringLiteral("Development")
        && m_activeWorkspace != QStringLiteral("Personal")) {
        m_activeWorkspace = QStringLiteral("Focus");
    }

    m_showSystemMetrics = settings.value(QStringLiteral("ui/showSystemMetrics"), true).toBool();

    const int savedInterval = settings.value(QStringLiteral("system/metricsRefreshIntervalMs"), 2000).toInt();
    if (savedInterval == 1000 || savedInterval == 2000 || savedInterval == 5000)
        m_metricsRefreshIntervalMs = savedInterval;
}

QString AppController::activeWorkspace() const
{
    return m_activeWorkspace;
}

bool AppController::showSystemMetrics() const
{
    return m_showSystemMetrics;
}

int AppController::metricsRefreshIntervalMs() const
{
    return m_metricsRefreshIntervalMs;
}

QString AppController::launchBrowser() const
{
    const bool requested = QDesktopServices::openUrl(QUrl(QStringLiteral("https://duckduckgo.com")));
    return requested
        ? QStringLiteral("Browser launch requested using your system default.")
        : QStringLiteral("Could not request the system browser to open.");
}

QString AppController::launchTerminal() const
{
    QString executable;
#ifdef Q_OS_WIN
    executable = QStandardPaths::findExecutable(QStringLiteral("wt.exe"));
    if (executable.isEmpty())
        executable = QStandardPaths::findExecutable(QStringLiteral("powershell.exe"));
    if (executable.isEmpty())
        executable = QStandardPaths::findExecutable(QStringLiteral("cmd.exe"));
#elif defined(Q_OS_MACOS)
    executable = QStandardPaths::findExecutable(QStringLiteral("open"));
    if (!executable.isEmpty()) {
        return QProcess::startDetached(executable, {QStringLiteral("-a"), QStringLiteral("Terminal")})
            ? QStringLiteral("Terminal launch requested.")
            : QStringLiteral("Could not start the Terminal application.");
    }
#else
    const QStringList candidates = {
        QStringLiteral("x-terminal-emulator"),
        QStringLiteral("konsole"),
        QStringLiteral("gnome-terminal"),
        QStringLiteral("xfce4-terminal"),
        QStringLiteral("kitty"),
        QStringLiteral("foot"),
        QStringLiteral("xterm")
    };
    for (const QString &candidate : candidates) {
        executable = QStandardPaths::findExecutable(candidate);
        if (!executable.isEmpty())
            break;
    }
#endif

    if (executable.isEmpty())
        return QStringLiteral("No supported terminal application was found.");

    return QProcess::startDetached(executable, {})
        ? QStringLiteral("Terminal launch requested.")
        : QStringLiteral("Could not start the terminal application.");
}

QString AppController::folderPath(const QString &folderId) const
{
    QStandardPaths::StandardLocation location = QStandardPaths::HomeLocation;

    if (folderId == QStringLiteral("desktop"))
        location = QStandardPaths::DesktopLocation;
    else if (folderId == QStringLiteral("documents"))
        location = QStandardPaths::DocumentsLocation;
    else if (folderId == QStringLiteral("downloads"))
        location = QStandardPaths::DownloadLocation;
    else if (folderId == QStringLiteral("pictures"))
        location = QStandardPaths::PicturesLocation;
    else if (folderId == QStringLiteral("music"))
        location = QStandardPaths::MusicLocation;
    else if (folderId == QStringLiteral("videos"))
        location = QStandardPaths::MoviesLocation;

    QString path = QStandardPaths::writableLocation(location);
    if (path.isEmpty())
        path = QDir::homePath();

    return QDir::cleanPath(path);
}

QString AppController::openFolder(const QString &folderId) const
{
    const QString path = folderPath(folderId);
    if (path.isEmpty())
        return QStringLiteral("This folder location is not available.");

    if (!QDir(path).exists() && !QDir().mkpath(path))
        return QStringLiteral("Could not access or create this folder.");

    const bool requested = QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    return requested
        ? QStringLiteral("File manager launch requested.")
        : QStringLiteral("Could not request the system file manager to open this folder.");
}

void AppController::setActiveWorkspace(const QString &workspace)
{
    if (workspace != QStringLiteral("Focus")
        && workspace != QStringLiteral("Development")
        && workspace != QStringLiteral("Personal")) {
        return;
    }

    if (m_activeWorkspace == workspace)
        return;

    m_activeWorkspace = workspace;
    QSettings settings;
    settings.setValue(QStringLiteral("workspace/active"), m_activeWorkspace);
    emit activeWorkspaceChanged();
}

void AppController::setShowSystemMetrics(bool enabled)
{
    if (m_showSystemMetrics == enabled)
        return;

    m_showSystemMetrics = enabled;
    QSettings settings;
    settings.setValue(QStringLiteral("ui/showSystemMetrics"), m_showSystemMetrics);
    emit showSystemMetricsChanged();
}

void AppController::setMetricsRefreshIntervalMs(int intervalMs)
{
    if (intervalMs != 1000 && intervalMs != 2000 && intervalMs != 5000)
        return;

    if (m_metricsRefreshIntervalMs == intervalMs)
        return;

    m_metricsRefreshIntervalMs = intervalMs;
    QSettings settings;
    settings.setValue(QStringLiteral("system/metricsRefreshIntervalMs"), m_metricsRefreshIntervalMs);
    emit metricsRefreshIntervalMsChanged();
}
