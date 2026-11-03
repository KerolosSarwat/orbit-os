# Architecture Decision Records

## ADR-001: Start from Linux rather than a custom kernel

**Status:** Accepted for prototype; review before public release.

**Decision:** Use a mainstream Linux kernel and existing distribution infrastructure.

**Why:** Hardware support, drivers, networking, filesystems and mature security facilities already exist. The project can focus on differentiated experience and reliability.

**Consequences:** ORBIT inherits upstream kernel and distribution maintenance responsibilities. The project must track security updates and clearly distinguish its own code from upstream components.

## ADR-002: Use Fedora 44 as the reference development platform

**Status:** Accepted for Alpha 0.1.

**Decision:** Use Fedora 44 x86-64 as the reference environment, initially with KDE Kinoite for desktop integration evaluation.

**Why:** It provides a current Linux base and an atomic desktop variant that is useful for testing update and rollback concepts.

**Review:** Compare Fedora Atomic and bootc workflows before choosing the public system-image build pipeline.

## ADR-003: Use Qt Quick / QML for the shell prototype

**Status:** Accepted for Alpha 0.1.

**Decision:** Use Qt 6, QML and CMake for the shell UI.

**Why:** Qt Quick supports fluid, declarative interfaces and can target multiple platforms. Keep the project compatible with Qt 6.11+; use Qt 6.12 LTS where available.

## ADR-004: Reuse KWin initially

**Status:** Accepted for Alpha 0.1.

**Decision:** Use KWin as the initial Wayland compositor and session infrastructure. Develop ORBIT shell UI separately.

**Why:** Writing a compositor would add significant complexity before the user experience is validated.

## ADR-005: No privileged operations in the shell UI

**Status:** Accepted.

**Decision:** QML must not execute arbitrary privileged commands or call system-changing operations directly. Future functionality must use a narrow service API with explicit authorization and auditability.
