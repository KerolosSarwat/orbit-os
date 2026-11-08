cmake_minimum_required(VERSION 3.21)

project(orbit-kernel VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(orbit-kernel
    src/kernel.c
    src/serial.c
)

target_include_directories(orbit-kernel PRIVATE
    include
)

target_compile_options(orbit-kernel PRIVATE
    -Wall
    -Wextra
    -Werror
    -ffreestanding
    -fno-builtin
    -fno-stack-protector
    -fno-pic
    -fno-pie
    -fno-asynchronous-unwind-tables
    -fno-unwind-tables
    -m64
    -mcmodel=kernel
    -mno-red-zone
    -mno-mmx
    -mno-sse
    -mno-sse2
)

target_link_options(orbit-kernel PRIVATE
    -nostdlib
    -static
    -no-pie
    -fuse-ld=lld
    "-Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/linker/x86_64.ld"
    "-Wl,-z,max-page-size=0x1000"
    "-Wl,--build-id=none"
    "-Wl,-Map,${CMAKE_CURRENT_BINARY_DIR}/orbit-kernel.map"
)

set_target_properties(orbit-kernel PROPERTIES
    OUTPUT_NAME "orbit-kernel"
    SUFFIX ".elf"
)

add_custom_target(orbit-iso
    COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/scripts/build-iso.sh"
            "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS orbit-kernel
    USES_TERMINAL
    COMMENT "Building bootable ORBIT K0.1 ISO"
)

add_custom_target(orbit-run
    COMMAND qemu-system-x86_64
            -machine q35
            -m 2G
            -cdrom "${CMAKE_CURRENT_BINARY_DIR}/orbit-k0.1.iso"
            -boot d
            -serial stdio
            -display none
            -no-reboot
            -no-shutdown
    DEPENDS orbit-iso
    USES_TERMINAL
    COMMENT "Booting ORBIT K0.1 in QEMU"
)
