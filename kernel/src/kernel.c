#include <stdint.h>
#include <limine.h>

#include "serial.h"

#define ORBIT_KERNEL_VERSION "0.1.0"

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[4] =
    LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[3] =
    LIMINE_BASE_REVISION(6);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_stack_size_request stack_size_request = {
    .id = LIMINE_STACK_SIZE_REQUEST_ID,
    .revision = 0,
    .stack_size = 64 * 1024
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_bootloader_info_request bootloader_info_request = {
    .id = LIMINE_BOOTLOADER_INFO_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memory_map_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[2] =
    LIMINE_REQUESTS_END_MARKER;

static __attribute__((noreturn)) void kernel_halt(void)
{
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}

static __attribute__((noreturn)) void kernel_panic(const char *message)
{
    serial_write_line("");
    serial_write("[PANIC] ");
    serial_write_line(message);
    kernel_halt();
}

static void print_memory_map(void)
{
    struct limine_memmap_response *response = memory_map_request.response;

    if (response == 0 || response->entries == 0) {
        kernel_panic("Limine did not provide a memory map.");
    }

    uint64_t usable_bytes = 0;

    serial_write("[MEM] Memory map entries: ");
    serial_write_u64(response->entry_count);
    serial_write_line("");

    for (uint64_t index = 0; index < response->entry_count; ++index) {
        const struct limine_memmap_entry *entry = response->entries[index];

        if (entry == 0) {
            continue;
        }

        if (entry->type == LIMINE_MEMMAP_USABLE) {
            if (UINT64_MAX - usable_bytes < entry->length) {
                usable_bytes = UINT64_MAX;
                break;
            }

            usable_bytes += entry->length;
        }
    }

    serial_write("[MEM] Usable RAM: ");
    serial_write_u64(usable_bytes);
    serial_write(" bytes (");
    serial_write_u64(usable_bytes / (1024 * 1024));
    serial_write_line(" MiB)");
}

static void print_bootloader_info(void)
{
    struct limine_bootloader_info_response *response =
        bootloader_info_request.response;

    if (response == 0) {
        serial_write_line("[BOOT] Bootloader information unavailable.");
        return;
    }

    serial_write("[BOOT] Bootloader: ");
    serial_write(response->name != 0 ? response->name : "unknown");
    serial_write(" ");
    serial_write_line(response->version != 0 ? response->version : "");
}

__attribute__((noreturn))
void _start(void)
{
    serial_init();

    serial_write_line("");
    serial_write_line("========================================");
    serial_write_line("          ORBIT OPERATING SYSTEM");
    serial_write_line("========================================");
    serial_write("Kernel Version: ");
    serial_write_line(ORBIT_KERNEL_VERSION);
    serial_write_line("Architecture: x86_64");
    serial_write_line("");

    if (!LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision)) {
        kernel_panic("Bootloader does not support Limine base revision 6.");
    }

    serial_write_line("[BOOT] Limine protocol base revision 6 accepted.");
    print_bootloader_info();

    if (hhdm_request.response != 0) {
        serial_write("[MM] Higher-half direct map offset: ");
        serial_write_hex(hhdm_request.response->offset);
        serial_write_line("");
    } else {
        serial_write_line("[MM] Higher-half direct map response unavailable.");
    }

    print_memory_map();

    serial_write_line("");
    serial_write_line("[OK] Kernel initialization complete.");
    serial_write_line("ORBIT KERNEL READY");

    kernel_halt();
}
