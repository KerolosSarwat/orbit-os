set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET x86_64-unknown-none-elf)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_FLAGS_INIT
    "-ffreestanding -fno-stack-protector -fno-pic -fno-pie -m64 -mcmodel=kernel -mno-red-zone"
)

set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")
