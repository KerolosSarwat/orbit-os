# ORBIT OS — Alpha 0.1 Starter

**Codename:** ORBIT  
**Product line:** Computing, without friction.  
**Status:** Early desktop-shell prototype; not an installable operating system yet.

ORBIT is an exploration of a calmer, more responsive, privacy-conscious desktop operating system. This repository is the first engineering step: a Qt Quick desktop-shell prototype that can be developed on a normal Linux desktop and later integrated into a dedicated Wayland session.

## What is included

- A Qt 6 / QML desktop-shell preview.
- ORBIT navigation rail, overview dashboard, workspace cards and app launcher tiles.
- A command-palette interaction (Ctrl+Space / Ctrl+K).
- A quick-settings preview panel.
- CMake + Ninja build configuration.
- Initial architecture and decision records.

## Important prototype limitations

This is **not yet an operating system** and does not replace KDE, Windows, or macOS. It runs as a regular desktop application inside an existing desktop session. Its launcher, workspace cards, quick settings, system metrics and command actions are UI demonstrations; they do not yet control host applications, hardware, user sessions or OS services. The next increments will connect these interfaces to real system components, then create a dedicated ORBIT session.

## Technology baseline

- Fedora Linux 44 as the initial reference environment.
- Qt 6.11+ (Qt 6.12 LTS is preferred when available).
- C++20 for the application entry point.
- QML / Qt Quick for the shell UI.
- CMake 3.21+ and Ninja.
- KWin / Wayland as the initial compositor and session infrastructure.

The project does not hard-require Qt 6.12; it is designed to build against compatible Qt 6 releases available in the selected Fedora package stream.

## Build on Fedora

Install the development tools:

~~~bash
sudo dnf install -y \
  git gcc-c++ cmake ninja-build \
  qt6-qtbase-devel qt6-qtdeclarative-devel
~~~

Configure and build:

~~~bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
~~~

Run the preview from your graphical desktop session:

~~~bash
./build/orbit-shell
~~~

If Qt Quick Controls QML modules are missing at runtime, install the Fedora package that provides the missing import for your Fedora release. Package splits can change between releases.

## Build in a Fedora development container (Podman)

Containerfile.dev provides a reproducible compiler/Qt environment. It is a development container, not an installable ORBIT system image.

~~~bash
podman build -f Containerfile.dev -t orbit-dev .
podman run --rm -it -v "$PWD:/workspace:Z" -w /workspace orbit-dev \
  bash -lc 'cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build --parallel'
~~~

Run the resulting graphical app from the host Fedora session for the simplest display integration.

## Build on Windows

For the first UI preview, install Qt 6.12 (or a compatible Qt 6 release), CMake and a supported C++ compiler using the Qt Online Installer / Qt Creator. Open this folder in Qt Creator and configure the CMake project. This previews the UI only; Fedora-specific OS-image and Wayland-session work should be done in a Linux environment.

## Recommended development environment

Use a Fedora 44 x86-64 virtual machine (or a spare test machine) for Linux integration work. Keep the host OS untouched. Do not test by replacing the host bootloader or desktop session during this alpha stage.

## Project structure

~~~text
orbit-os/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── src/main.cpp
├── qml/Main.qml
├── docs/
│   ├── ARCHITECTURE.md
│   ├── PRODUCT_VISION.md
│   └── DECISIONS.md
└── scripts/build-fedora.sh
~~~

## Quality gates for Alpha 0.1

- [ ] Configure and build with Qt 6.11+.
- [ ] Launch on a Fedora KDE/Wayland session.
- [ ] Confirm UI scaling at 1280×720 and 1920×1080.
- [ ] Confirm keyboard navigation and command palette shortcuts.
- [ ] Confirm no shell crash during repeated open/close interactions.
- [ ] Record baseline memory use, idle CPU use and first-frame time.
- [ ] Create a separate test VM snapshot before any system-integration experiment.

## Contributing

Read docs/ARCHITECTURE.md and docs/DECISIONS.md before introducing system-level dependencies. Prefer small components with clear interfaces. Never run privileged host commands directly from the QML UI; system actions must go through narrowly scoped, permission-checked services.

## License

The original ORBIT starter code is licensed under the MIT License. Third-party components retain their own licenses. This license does not relicense Linux, KDE, Qt or other dependencies.
