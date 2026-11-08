# ORBIT Kernel — K0.1 First Boot

This is the first custom-kernel development track for ORBIT. It is intentionally isolated from the existing Qt/QML desktop prototype.

## Target

- x86_64
- Freestanding C11
- Limine boot protocol
- Higher-half kernel linked at 0xffffffff80000000
- CMake + Ninja
- Clang/LLD
- QEMU
- COM1 serial diagnostics

## K0.1 scope

- Limine entry point and protocol requests
- Kernel version and bootloader identification
- COM1 serial output
- Limine memory-map inspection
- Higher-half direct-map offset reporting
- Minimal panic and halt paths
- BIOS/UEFI bootable ISO generation

This milestone does not yet install a custom GDT/IDT, replace the bootloader's page tables, enable multitasking, or run user-space programs. Those belong to later milestones.

## Build environment (Ubuntu / WSL2)

Install the host tools:

```bash
sudo apt update
sudo apt install -y clang lld cmake ninja-build qemu-system-x86 xorriso mtools git
```

Download a Limine binary release from:

https://github.com/limine-bootloader/limine/releases

Extract it to `kernel/vendor/limine`, or set `LIMINE_DIR` to the extracted directory. The directory must contain:

- `limine`
- `limine-bios.sys`
- `limine-bios-cd.bin`
- `limine-uefi-cd.bin`
- `BOOTX64.EFI`

## Configure and compile

From the repository root:

```bash
cmake -S kernel -B kernel/build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=kernel/toolchain-x86_64.cmake

cmake --build kernel/build
```

## Build the ISO

If Limine was extracted to `kernel/vendor/limine`:

```bash
cmake --build kernel/build --target orbit-iso
```

Or specify another directory:

```bash
LIMINE_DIR=/path/to/limine-binary-release \
  cmake --build kernel/build --target orbit-iso
```

## Run in QEMU

```bash
cmake --build kernel/build --target orbit-run
```

QEMU sends the emulated COM1 serial port to the terminal. A successful K0.1 boot ends with:

```text
[OK] Kernel initialization complete.
ORBIT KERNEL READY
```

## Notes

- The root repository CMake project remains the Qt desktop application.
- Always configure the kernel using `kernel/CMakeLists.txt`, not the root CMake project.
- Do not install this ISO on physical hardware during K0.1.
- The Limine protocol header is sourced from the official Limine protocol repository and retains its 0BSD notice.
