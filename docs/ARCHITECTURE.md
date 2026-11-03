# ORBIT Technical Architecture — Initial Proposal

## Architecture direction

Start with a Linux-based distribution and develop an ORBIT desktop shell and system services. Do not write a custom kernel or compositor for the initial release.

## Layer model

~~~text
ORBIT Experience
  ├── Desktop Shell (Qt Quick / QML)
  ├── ORBIT Command
  ├── ORBIT Spaces
  └── Control Center
        │
        ▼
ORBIT User Services (Rust where appropriate)
  ├── Settings and preferences
  ├── Workspace state
  ├── Notifications
  ├── Permission broker
  ├── Update and recovery UI/API
  └── AI orchestration (later, opt-in)
        │
        ▼
Desktop and System Infrastructure
  ├── Wayland protocol
  ├── KWin compositor/session (initially)
  ├── systemd user/system services
  ├── D-Bus
  ├── PipeWire
  └── NetworkManager
        │
        ▼
OS Image and Kernel
  ├── Fedora Atomic / bootc evaluation
  ├── Linux kernel
  ├── SELinux
  ├── Flatpak
  └── Hardware drivers
~~~

## Alpha 0.1 implementation

The current orbit-shell is a regular Qt Quick application. It runs inside an existing desktop session and intentionally avoids privileged operations. The shell is not yet a Wayland compositor, login manager, panel replacement, or system service.

## Planned boundaries

- QML renders UI and handles local presentation state only.
- System operations will be exposed by a small, typed service API.
- Privileged operations must never be implemented as arbitrary shell commands supplied by QML or AI.
- Each service should have explicit permissions, logging and tests.
- User data must be kept separate from the immutable/recoverable system image.

## Compatibility approach

- Linux applications: native packages and Flatpak.
- Windows applications: evaluate Wine/Proton per application; no blanket compatibility promise.
- Android applications: optional experimental layer after platform review.
- macOS applications: no general compatibility commitment.

## Reliability approach

Evaluate bootable container / image-based system updates, staged deployments, rollback and an independent user-data backup mechanism. A system-image rollback must not be described as restoring deleted personal files.

## Security approach

Use Secure Boot where supported, full-disk encryption, SELinux enforcing, application sandboxing, signed updates and least-privilege service interfaces. Security controls must be validated on the target image rather than assumed from package selection alone.
