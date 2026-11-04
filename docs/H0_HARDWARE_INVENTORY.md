# ORBIT H0 — Hardware Inventory Design

**Status:** Design specification; H0.1 data contract committed, build verification pending  
**Reference profile:** ORBIT-REF-LAPTOP-01  
**Scope:** Read-only hardware discovery; no tuning or system changes.

## 1. Purpose

H0 establishes a trustworthy baseline of the computer running ORBIT. It identifies hardware, collects capabilities exposed by firmware and the operating system, discovers driver bindings, and distinguishes advertised capabilities from those verified at runtime.

H0 is an observer. It must not change CPU/GPU frequencies, power limits, fan curves, firmware settings, Secure Boot, storage configuration or device power states. Baseline scanning must not require administrator/root privileges. Unknown values remain unknown; they are never guessed from a product name.

The first runnable collector targets Windows because that is the current ORBIT development environment. The data model remains platform-neutral so a Fedora/Linux collector can later populate the same schema.

## 2. Reference machine

| Field | Expected configuration | Source / qualification |
|---|---|---|
| Form factor | Laptop | User-provided |
| CPU | Intel Core i7-13620H | User-provided; verify on device |
| CPU topology | 10 cores: 6 Performance + 4 Efficient; 16 threads | Intel specification; detect actual OS topology |
| CPU maximum turbo | Up to 4.90 GHz | Product specification, not sustained speed |
| CPU processor base power | 45 W | Product specification, not actual laptop power limit |
| CPU maximum turbo power | Up to 115 W | Intel product specification; not the laptop's measured sustained limit |
| Integrated GPU | Intel UHD Graphics for 13th Gen Intel Processors | Expected from CPU SKU; verify device ID |
| Discrete GPU | NVIDIA GeForce RTX 4050 Laptop GPU | User-provided; verify device ID |
| Discrete GPU memory | 6 GB GDDR6 | User-provided; verify driver-reported memory |
| Installed RAM | 16 GB | User-provided; measure installed and OS-usable separately |

H0 must discover the laptop manufacturer, exact model/SKU, board, BIOS/UEFI, Secure Boot and TPM state; actual RTX 4050 power configuration and driver; RAM type/speed/modules/channels; storage model/interface/firmware; wireless, Bluetooth, audio, camera, USB and display devices; battery data; and available thermal/fan sensors.

NVIDIA lists the RTX 4050 Laptop GPU family with a 35–115 W GPU subsystem power range. This is a family range, not this laptop's configured TGP. Laptop RTX 4050 power limits vary by implementation. Do not assume a TGP or performance level from the GPU name. CPU advertised power figures are reference specifications, not live operating limits.

## 3. Discovery contract and evidence

Every property is an evidence-bearing value containing a value/unit, normalized property name, source, observation time, evidence state, confidence and optional note.

Evidence states:

| State | Meaning |
|---|---|
| Present | Device identity found in hardware or firmware inventory. |
| Described | Firmware or specification advertises the capability. |
| Driver-bound | An OS driver is associated with the device. |
| Runtime-accessible | Relevant OS/driver interface can be queried. |
| Verified | A safe runtime check confirmed the capability. |
| Not reported | No authoritative source exposed the value. |
| Permission denied | The source exists but access was denied. |
| Probe skipped | Query deliberately skipped to avoid side effects, such as waking a GPU. |
| Unsupported | No provider exists for this platform/property. |
| Error | Probe failed; retain a concise error code only. |

Confidence is high, medium or low and describes the quality of evidence. It is not a hardware quality score. A device may be Present but not Driver-bound; never collapse these into one “supported” flag.

## 4. Normalized snapshot

Use a schema version independent of the app version. Top-level sections:

1. Scan metadata: schema version, scan ID/time, collector version and OS/platform.
2. Machine: manufacturer, product, SKU, board, firmware, boot mode, Secure Boot and TPM.
3. Processors: identity, package, core/thread topology, P/E classification, caches and instruction sets.
4. Graphics: adapters, vendor/device IDs, integrated/discrete role, memory, driver, display role, APIs and power-state evidence.
5. Memory: installed and usable capacity, module facts, memory type/speed/channel data when exposed.
6. Storage: devices, interface, capacity and firmware; health only where safely available.
7. Power/thermals: battery, AC state, sensors, thermal zones and fan telemetry where exposed.
8. Connectivity/peripherals: PCI/USB, Wi-Fi, Bluetooth, audio, camera and controllers.
9. Security: TPM state, Secure Boot state and virtualization capability/exposure.
10. Limitations: unavailable fields, skipped probes and collection warnings.

Use explicit units: bytes, hertz, milliwatts, millidegrees Celsius and seconds. UI formatting may convert units. Distinguish physical/installed RAM from OS-usable memory and dedicated VRAM from shared memory or OS memory budgets.

Do not include serial numbers, system UUIDs, MAC addresses, Windows product keys, usernames or full device paths in the default exported report. Redact identifiers unless the user explicitly chooses to include them.

## 5. Collectors

### 5.1 Windows — first implementation

The baseline collector must not depend on shelling out to PowerShell, WMIC, dxdiag or third-party utilities. Prefer stable APIs:

| Area | Preferred source |
|---|---|
| Manufacturer/model/board/BIOS | Windows firmware table API (SMBIOS/RSMB); selected Win32 APIs as fallback |
| CPU topology | GetLogicalProcessorInformationEx; GetSystemCpuSetInformation where available |
| CPU P/E classification | Efficiency-class data when exposed by Windows; otherwise not reported |
| CPU identity/features | OS processor identity and CPUID-backed facts as supplemental evidence |
| Installed RAM | GetPhysicallyInstalledSystemMemory where supported |
| Usable/available RAM | GlobalMemoryStatusEx |
| Memory modules/type/speed | SMBIOS Type 16/17 where present; label as firmware-reported |
| GPU enumeration | DXGI adapter enumeration plus Windows device inventory/SetupAPI IDs |
| GPU driver | Windows device properties; optional vendor interfaces |
| GPU memory | DXGI dedicated/shared values with their semantics clearly labelled |
| Storage | Windows storage/device enumeration and volume APIs |
| Battery/AC | Windows power and supported battery interfaces |
| TPM/Secure Boot | Supported Windows security/platform APIs |
| Network/audio/USB/camera | Windows device inventory and device properties |
| Thermal/fans | Optional supported interfaces only; never vendor-specific writes |

WMI may be a fallback for fields with no well-defined API, but source and firmware-reporting limitations must be visible. Do not parse localized human-readable command output.

### 5.2 Fedora/Linux — target provider

Use kernel/system interfaces, not command output:

| Area | Preferred source |
|---|---|
| CPU topology | /sys/devices/system/cpu; /proc/cpuinfo as fallback |
| CPU frequency | CPUFreq sysfs |
| Hybrid CPU capacity | Kernel topology/capacity attributes when exposed |
| PCI devices and drivers | /sys/bus/pci/devices, driver symlinks, libudev |
| GPU/display | DRM sysfs, card/render nodes and driver bindings |
| NVIDIA runtime facts | Optional NVML provider when a supported NVIDIA driver is installed |
| RAM | /proc/meminfo; DMI data where available and permitted |
| Storage | /sys/block, /sys/class/block and udev |
| Battery | /sys/class/power_supply |
| Thermal/fans | /sys/class/thermal and /sys/class/hwmon |
| TPM | /sys/class/tpm and supported TPM interfaces |
| Secure Boot/firmware | EFI variables and platform interfaces, permission-aware |
| Hotplug | libudev events |

Do not run lspci, nvidia-smi, a benchmark or a Vulkan workload during passive baseline discovery. Some queries can wake a discrete GPU. A later user-requested diagnostic may warn that it can activate the device.

### 5.3 Shared safety rules

- No shell commands for normal inventory and no network calls.
- No administrator/root requirement for baseline collection.
- No writes to sysfs, registry, firmware, device configuration or power-policy interfaces.
- No benchmarks or stress tests.
- Do not wake a sleeping discrete GPU merely to collect cosmetic details.
- Record why a field is missing whenever the collector can determine the reason.
- Optional probes need timeouts and cancellation; scan work must not block UI startup.

## 6. Hybrid graphics requirements

The reference laptop has Intel integrated graphics and an NVIDIA discrete GPU. H0 must report two separate adapters and must not assume that NVIDIA drives the internal display.

Collect, where available: which GPU drives each display; integrated/discrete role; driver and version; graphics API availability; dedicated/shared memory with correct labels; current power state; and whether a probe was skipped to preserve low-power state.

H0 remains observational. The later Linux integration should cooperate with PRIME/offload and runtime power management rather than keeping the discrete GPU permanently active.

## 7. Proposed C++ source layout

Keep inventory separate from the existing SystemMonitor class. SystemMonitor handles frequently refreshed usage metrics; Hardware Inventory is a slower machine identity/capability snapshot.

- src/hardware/HardwareInventoryService.h/.cpp — scan orchestration, snapshot lifecycle and signals.
- src/hardware/HardwareInventorySnapshot.h — normalized data and evidence metadata.
- src/hardware/HardwareProbe.h — platform-provider contract.
- src/hardware/WindowsHardwareProbe.h/.cpp — first collector.
- src/hardware/LinuxHardwareProbe.h/.cpp — later Fedora/Linux collector.
- src/hardware/InventorySerializer.h/.cpp — versioned JSON export.
- qml/pages/HardwarePage.qml — Inspector interface.
- tests/hardware/ — parser, fixture and schema tests.

The Qt-facing service should expose startScan(), scanInProgress, lastSnapshot, lastScanTime and scanCompleted/scanFailed signals. Native APIs stay out of QML. Scan on a worker thread, support cancellation, preserve the last successful snapshot after partial failure and never delay shell startup.

## 8. Inspector UI

### Overview
Show laptop manufacturer/model (or “Model not reported”), CPU/topology, both GPUs, installed versus usable RAM, primary storage, firmware/security summary, last scan time and warnings.

### Component details
- Processor: topology, cache, instruction sets and separately labelled specification/runtime values.
- Graphics: adapters, VRAM, driver, display routing, APIs and power-state evidence.
- Memory: capacity, modules, type, speed and channel evidence.
- Storage: model, capacity, interface and firmware.
- Power & thermals: battery/AC state, exposed temperatures and fan telemetry.
- Devices: Wi-Fi, Bluetooth, Ethernet, audio, camera, USB and input controllers.
- Security: TPM, Secure Boot and virtualization capability/state.

Use labels such as Detected, Driver loaded, Available, Verified, Not reported and Probe skipped. Do not show “healthy” merely because a device exists. Provide Rescan and explicit local Export report actions. Default export is redacted.

## 9. Implementation sequence

### H0.1 — Data contract and fixtures
Define schema v1 and evidence types. Add sample snapshots for full and partial inventory. Add parser tests for SMBIOS, CPU topology and device identifiers. The collector must not contain hard-coded product assumptions.

### H0.2 — Windows baseline collector
Collect machine/firmware identity, CPU and topology, installed/usable RAM, GPU enumeration/driver binding, storage, core devices, and security/battery fields where reliable. Represent unknowns honestly.

### H0.3 — Inspector page
Add overview/detail views, scan time/limitations, rescan and redacted JSON export. Keep inventory separate from live telemetry.

### H0.4 — Validate the reference laptop
- CPU matches Intel Core i7-13620H.
- OS-visible topology is consistent with 10 cores / 16 threads; P/E split appears only when exposed.
- Intel integrated graphics and NVIDIA RTX 4050 are both identified.
- 6 GB dedicated memory is reported where the driver exposes it.
- Installed RAM is approximately 16 GB; usable memory is separate.
- Laptop model, BIOS, storage, wireless, battery and display details are populated where available.
- Baseline scanning does not intentionally wake the NVIDIA GPU.
- No system setting or device configuration changes.

### H0.5 — Fedora/Linux collector
Implement sysfs/udev providers and validate that Linux populates the same schema. The two operating systems need comparable concepts, not artificially identical detail.

## 10. Acceptance criteria

H0 is complete when a scan returns a schema-valid snapshot without blocking the desktop; all reported fields have a source and evidence state; missing/inaccessible values are explicit; both GPUs and the reference CPU are identified; installed and usable RAM are distinct; passive scanning does not intentionally wake the dGPU; exports redact sensitive identifiers; baseline scans require no elevated privilege and make no system changes; and automated tests cover partial inventory, unknown devices, missing drivers, permission denial and schema compatibility.

## 11. Out of scope

H0 does not tune/undervolt/overclock, control fans, change thermal limits, update/configure firmware, install/remove drivers, benchmark performance, apply AI optimization, upload data to the cloud, fingerprint devices centrally or grant ORBIT Certified status. Certification requires a later repeatable hardware qualification matrix.

**H0 produces trustworthy facts. Later phases may use those facts to select safe policies, but must never confuse catalog specifications with the actual capabilities or limits of this individual laptop.**

## 12. Implementation status

- **H0.1 — Data contract:** C++ evidence/state types, normalized snapshot model, JSON serializer/deserializer, fixture-based contract tests and CTest/CI integration have been committed.
- **H0.2 — Windows baseline collector:** Not started. No hardware is being scanned by the application yet.
- **H0.3 — Inspector UI:** Not started.
- **Build/test verification:** Source and workflow changes are committed, but a successful Qt build and CTest run have not yet been confirmed on the Windows Qt 6.12/MinGW development kit or in GitHub Actions.

## 13. H0.2 implementation

The first Windows implementation is committed as a passive collector:

- `WindowsHardwareProbe` reads CPU brand/vendor from the Windows processor registry metadata and core/thread topology through `GetLogicalProcessorInformationEx(RelationProcessorCore)`.
- The collector counts all Windows efficiency classes, including class zero. It reports P-core/E-core counts only when more than one efficiency class is exposed; otherwise those classifications remain not reported.
- Installed RAM is read through `GetPhysicallyInstalledSystemMemory`; OS-usable and currently available RAM are separate `GlobalMemoryStatusEx` values.
- SMBIOS Type 0, Type 1 and Type 2 records are parsed for BIOS, system product and baseboard strings. Serial numbers and UUID fields are deliberately not read or exported.
- Present display-class devices are enumerated through SetupAPI. The report records descriptions, manufacturer strings, PCI vendor/device IDs, PnP hardware ID and the associated driver registry key where available.
- GPU role is intentionally left unknown during this first passive scan. Vendor identity alone does not prove whether an adapter is integrated or discrete.
- DXGI runtime memory queries are deferred; dedicated/shared video-memory values are marked `probe-skipped` to avoid initializing a sleeping discrete GPU.
- Storage, battery, thermal, fan, full driver version, display routing, network and peripheral details are not yet collected.

A standalone Windows-only utility, `orbit-hardware-scan`, writes a JSON snapshot to stdout or to a path supplied as its first argument. The Windows collector source is also compiled by the hardware contract test target on Windows, so the test-target build exercises the native provider's compile path.

### H0.2 verification status

The Windows collector and scanner are committed but have **not yet been compiled or run on the reference laptop**. The Linux build compiles only the non-Windows provider stub. The next validation must be performed with the user's Qt 6.12 MinGW Windows kit.
