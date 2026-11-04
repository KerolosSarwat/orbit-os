# ORBIT OS — Alpha 0.2

**Codename:** ORBIT  
**Product line:** Computing, without friction.  
**Status:** Cross-platform desktop-shell prototype; not an installable operating system.

ORBIT is an exploration of a calmer, more responsive, privacy-conscious desktop operating system. Alpha 0.2 moves the shell from a mostly static UI preview to a desktop application that can interact with the current host environment.

## Alpha 0.2 features

- Launch the system default web browser.
- Open common folders in the host operating system's file manager.
- Launch an installed system terminal (Windows Terminal / PowerShell / Command Prompt on Windows; common terminal emulators on Linux).
- Navigate between functional Overview, Workspaces, Applications, Files and Settings pages.
- Switch among Focus, Development and Personal ORBIT workspaces. The selected workspace is saved locally.
- Read live CPU, physical-memory and system-volume usage on Windows and Linux.
- Configure metrics visibility and refresh interval; preferences are saved locally.
- Open the command palette with Ctrl+Space or Ctrl+K.

## Hardware inventory foundation

- H0.1 provides a platform-neutral inventory model, evidence states, JSON serialization and fixture-based contract tests.
- H0.2 adds a passive Windows hardware collector and the Windows-only `orbit-hardware-scan` report utility. It is committed but still requires compilation and execution on a Windows development kit.
- The desktop shell does not yet display this hardware inventory; the scanner is a separate utility at this stage.

## Important scope boundaries

This is **not an installable operating system** and does not replace Windows, Linux, KDE or macOS. It runs as an ordinary Qt desktop application.

ORBIT Spaces in Alpha 0.2 are persistent in-app work contexts. They do not yet switch native Windows or Linux virtual desktops.

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
