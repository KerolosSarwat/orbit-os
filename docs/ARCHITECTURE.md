# ORBIT Technical Architecture — Alpha 0.2

## Runtime structure

Alpha 0.2 remains a normal Qt desktop application:

- **QML shell:** navigation, pages, command palette and visual presentation.
- **AppController:** controlled host integrations and locally persisted preferences.
- **SystemMonitor:** read-only CPU, physical memory and root-volume metrics.
- **Qt platform services:** default browser, file manager, storage metadata and application process creation.

## Host integration boundary

QML does not run shell commands. It calls a small set of explicit C++ methods:

- launchBrowser() opens a fixed HTTPS destination through QDesktopServices.
- openFolder(id) maps a known folder identifier to a QStandardPaths location, then requests the host file manager to open it.
- launchTerminal() searches a fixed allowlist of terminal executables and starts the first available executable.
- Preference setters validate values and persist them through QSettings.

Do not add arbitrary command strings, shell evaluation, or privileged actions to this API.

## System metrics

- Windows: GetSystemTimes for CPU deltas and GlobalMemoryStatusEx for physical memory.
- Linux: /proc/stat for CPU deltas and /proc/meminfo for memory.
- Both platforms: QStorageInfo for root-volume capacity and free-space metadata.

These are read-only, best-effort metrics. The first CPU sample establishes a baseline, so CPU usage may show "Measuring…" until the next refresh. Disk metrics describe the root volume only in this milestone.

## Workspace semantics

Focus, Development and Personal are persistent ORBIT application contexts. The selected context is saved using QSettings. This is not yet native OS virtual desktop control; implementing that requires a separate Wayland/KWin or Windows integration design.

## Next architecture steps

- Replace static quick-launch definitions with a desktop application index.
- Add a real file browser inside ORBIT only after filesystem permissions and navigation behavior are specified.
- Add native workspace/compositor integration only after the shell runs as a dedicated desktop session.
- Keep system metrics behind a testable interface and add unit tests for parsing and calculations.
