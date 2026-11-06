# ORBIT OS — Alpha 0.4

**Codename:** ORBIT  
**Product line:** Computing, without friction.  
**Status:** Cross-platform desktop-shell prototype; not an installable operating system.

ORBIT is an exploration of a calmer, more responsive, privacy-conscious desktop operating system. Alpha 0.4 extends the in-shell Hardware Inspector with Windows storage, battery/power, active display, connectivity and driver metadata discovery while keeping the project a desktop application prototype.

## Alpha 0.4 desktop features

- Launch the system default web browser.
- Open common folders in the host operating system's file manager.
- Launch an installed system terminal (Windows Terminal / PowerShell / Command Prompt on Windows; common terminal emulators on Linux).
- Navigate between functional Overview, Workspaces, Applications, Files, Settings and Hardware Inspector pages.
- Switch among Focus, Development and Personal ORBIT workspaces. The selected workspace is saved locally.
- Read live CPU, physical-memory and system-volume usage on Windows and Linux.
- Configure metrics visibility and refresh interval; preferences are saved locally.
- Open the command palette with Ctrl+Space or Ctrl+K; search for Hardware to open the Inspector.
- Scan Windows hardware asynchronously using read-only system APIs, validate the resulting evidence snapshot, and inspect machine identity, CPU topology, graphics adapters and memory.
- Export the latest validated hardware inventory as JSON to `Documents/ORBIT/HardwareReports`.

## Hardware inventory and Inspector

- H0.1 provides a platform-neutral inventory model, evidence states, JSON serialization and fixture-based contract tests.
- H0.2 provides a passive Windows hardware collector and the standalone `orbit-hardware-scan` utility. The collector has been run on the MSI Thin 15 B13VE reference laptop; CPU topology was verified at 10 physical cores / 16 logical processors (6 performance-class + 4 efficiency-class).
- H0.3 integrates a read-only Hardware Inspector page into the desktop shell. It runs scans off the UI thread, validates the snapshot before display, preserves the last valid inventory after a failed rescan, and exports JSON locally.
- The first live collector is Windows-only. On other platforms the Inspector explains that no native collector is included yet; it does not fabricate hardware values.
- The Inspector distinguishes present, described, driver-bound, verified and unavailable/skipped evidence. Not-collected data is never presented as proof that a device is absent.

## Important scope boundaries

This is **not an installable operating system** and does not replace Windows, Linux, KDE or macOS. It runs as an ordinary Qt desktop application.

ORBIT Spaces in Alpha 0.3 are persistent in-app work contexts. They do not yet switch native Windows or Linux virtual desktops.

The Browser, Files and Terminal actions request launch through the host operating system. Qt reports whether it could submit the request; it cannot guarantee that the external application finishes launching.

System metrics currently use Windows system APIs or Linux /proc data plus Qt storage information. Hardware switches, native compositor integration, an installer, system-wide settings and an OS login session are not included.

## Technology baseline

- Qt 6.12 LTS recommended; Qt 6.5+ supported by the project configuration.
- C++20 for host integration and metrics.
- QML / Qt Quick for the desktop shell.
- CMake 3.21+ and Ninja.
- Windows 10/11 and Fedora/Linux x86-64 are the initial development targets.

## Build on Windows with Qt Creator

1. Install Qt 6.12 with a matching MinGW 64-bit kit, Qt Quick/QML modules, CMake and Ninja using the Qt Online Installer.
2. Install Git for Windows.
3. Clone this repository:

~~~powershell
git clone https://github.com/KerolosSarwat/orbit-os.git
cd orbit-os
~~~

4. Open CMakeLists.txt in Qt Creator.
5. Select the Desktop Qt 6.12 MinGW 64-bit kit.
6. Configure, build, and run orbit-shell.

If you already have the repository cloned, update it first:

~~~powershell
git pull origin main
~~~

## Build on Fedora/Linux

Install the build and Qt packages:

~~~bash
sudo dnf install -y \
  git gcc-c++ cmake ninja-build \
  qt6-qtbase-devel qt6-qtdeclarative-devel
~~~

Configure, build and run:

~~~bash
git clone https://github.com/KerolosSarwat/orbit-os.git
cd orbit-os
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
./build/orbit-shell
~~~

## Development safety

- Test Linux integration in a disposable Fedora virtual machine.
- Do not replace the host bootloader or desktop session during this alpha stage.
- QML must not execute arbitrary privileged commands.
- Any future system-changing operation must go through a narrow, permission-checked service API.
- Metrics are informational and should not be treated as security or diagnostic guarantees.

## License

The original ORBIT starter code is licensed under the MIT License. Third-party components retain their own licenses.
