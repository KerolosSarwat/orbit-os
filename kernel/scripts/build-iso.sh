#!/usr/bin/env bash
set -euo pipefail

KERNEL_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-$KERNEL_DIR/build}"
LIMINE_DIR="${LIMINE_DIR:-$KERNEL_DIR/vendor/limine}"
ISO_ROOT="$BUILD_DIR/iso-root"
ISO_PATH="$BUILD_DIR/orbit-k0.1.iso"

required_files=(
    "$LIMINE_DIR/limine"
    "$LIMINE_DIR/limine-bios.sys"
    "$LIMINE_DIR/limine-bios-cd.bin"
    "$LIMINE_DIR/limine-uefi-cd.bin"
    "$LIMINE_DIR/BOOTX64.EFI"
)

for file in "${required_files[@]}"; do
    if [[ ! -f "$file" ]]; then
        echo "Missing Limine binary: $file" >&2
        echo "Download a Limine binary release and set LIMINE_DIR to its extracted directory." >&2
        exit 1
    fi
done

if [[ ! -f "$BUILD_DIR/orbit-kernel.elf" ]]; then
    echo "Kernel ELF not found. Build the orbit-kernel target first." >&2
    exit 1
fi

command -v xorriso >/dev/null 2>&1 || {
    echo "xorriso is required to create the bootable ISO." >&2
    exit 1
}

rm -rf "$ISO_ROOT"
mkdir -p "$ISO_ROOT/boot/limine" "$ISO_ROOT/EFI/BOOT"

cp "$BUILD_DIR/orbit-kernel.elf" "$ISO_ROOT/boot/orbit-kernel.elf"
cp "$KERNEL_DIR/boot/limine.conf" "$ISO_ROOT/boot/limine/limine.conf"
cp "$LIMINE_DIR/limine-bios.sys" "$ISO_ROOT/boot/limine/"
cp "$LIMINE_DIR/limine-bios-cd.bin" "$ISO_ROOT/boot/limine/"
cp "$LIMINE_DIR/limine-uefi-cd.bin" "$ISO_ROOT/boot/limine/"
cp "$LIMINE_DIR/BOOTX64.EFI" "$ISO_ROOT/EFI/BOOT/BOOTX64.EFI"

xorriso -as mkisofs \
    -R -r -J \
    -b boot/limine/limine-bios-cd.bin \
    -no-emul-boot -boot-load-size 4 -boot-info-table \
    --efi-boot boot/limine/limine-uefi-cd.bin \
    -efi-boot-part --efi-boot-image --protective-msdos-label \
    "$ISO_ROOT" -o "$ISO_PATH"

"$LIMINE_DIR/limine" bios-install "$ISO_PATH"

echo "Created: $ISO_PATH"
