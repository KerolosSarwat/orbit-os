# ORBIT Hardware Goal

## Core goal: tight hardware coupling

ORBIT will be designed and optimized in close cooperation with selected hardware platforms. The operating system and the machine should behave as one coherent product, delivering responsive graphics, dependable boot and resume, efficient power use, predictable thermals, reliable peripherals and integrated platform security.

### Strategy

- Keep Linux and established hardware standards as the compatibility foundation.
- Select a small number of x86-64 reference laptops and desktops for deep qualification.
- Detect actual device capabilities and IDs; do not rely on product names alone.
- Prefer upstream kernel drivers and firmware packages.
- Put ORBIT-specific tuning in documented, versioned, reversible platform profiles.
- Provide safe generic defaults for compatible hardware outside the certified list.
- Measure performance, battery, thermals, graphics and reliability on physical hardware.

### Hardware support tiers

1. **ORBIT Certified:** exact model and firmware configuration has passed the ORBIT hardware test matrix.
2. **ORBIT Compatible:** works with supported drivers and generic settings, but has not completed full qualification.
3. **Experimental / Unsupported:** known gaps or unvalidated components; no production support promise.

### Integration areas

Firmware and UEFI; CPU topology and scheduling; CPU frequency and power policy; GPU acceleration and display; storage and memory; Wi-Fi, Bluetooth and audio; camera, sensors and input devices; battery, cooling and suspend/resume; TPM, Secure Boot and encryption.

### Safety and engineering boundaries

Never silently overclock or undervolt, disable thermal safeguards, weaken security, or apply undocumented firmware changes. Platform settings must be transparent, measured, reversible and applied only through narrowly scoped, permission-checked services. Generic fallback behavior must remain available.

### First implementation sequence

H0: inventory candidate machines and device IDs.  
H1: choose one or two reference systems that can be tested repeatedly.  
H2: build read-only hardware discovery and capability reporting.  
H3: qualify drivers, graphics, connectivity, audio, power, thermals, sleep/resume and recovery.  
H4: introduce versioned platform profiles only after measured validation.

### Current status

This is a product and architecture goal, not an implemented feature in Alpha 0.2. The existing prototype reports basic CPU, memory and storage metrics only.
