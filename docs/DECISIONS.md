# Architecture Decision Records

## ADR-001: Start from Linux rather than a custom kernel

**Status:** Accepted for prototype; review before public release.

Use a mainstream Linux kernel and existing distribution infrastructure to focus project effort on user experience, compatibility and reliability.

## ADR-002: Use Fedora as the reference Linux environment

**Status:** Accepted for initial Linux integration.

Use Fedora KDE/Atomic as a development and integration target. Windows remains supported as the primary UI-development host.

## ADR-003: Use Qt Quick / QML for the shell

**Status:** Accepted.

Use Qt 6, QML and CMake for the cross-platform prototype. Qt 6.12 LTS is recommended; the project currently declares a Qt 6.5 minimum.

## ADR-004: Reuse existing desktop services

**Status:** Accepted.

Use QDesktopServices, QStandardPaths, QProcess, and Qt storage APIs rather than hard-coding all host integration in QML.

## ADR-005: No privileged operations in the shell UI

**Status:** Accepted.

QML must not execute arbitrary privileged commands. System operations must use narrow, typed and permission-checked APIs.

## ADR-006: Keep Alpha 0.2 workspaces application-scoped

**Status:** Accepted for Alpha 0.2.

Spaces are persistent ORBIT contexts, not claims of native virtual desktop management. Native desktop integration is deferred until the shell has a dedicated session.

## ADR-007: Read-only, cross-platform metrics

**Status:** Accepted for Alpha 0.2.

Use Windows system APIs and Linux procfs for CPU/memory metrics, plus QStorageInfo for storage. No privileged access is required.
