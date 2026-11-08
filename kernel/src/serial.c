#include "serial.h"

#define COM1_PORT 0x3F8

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void serial_init(void)
{
    outb(COM1_PORT + 1, 0x00); /* Disable UART interrupts. */
    outb(COM1_PORT + 3, 0x80); /* Enable divisor latch. */
    outb(COM1_PORT + 0, 0x01); /* 115200 baud, divisor 1. */
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x03); /* 8 data bits, no parity, one stop bit. */
    outb(COM1_PORT + 2, 0xC7); /* Enable FIFO and clear buffers. */
    outb(COM1_PORT + 4, 0x0B); /* IRQs enabled, RTS/DSR set. */
}

void serial_write_char(char character)
{
    /* Bound the wait so a missing UART cannot hang kernel startup forever. */
    for (uint32_t attempt = 0; attempt < 1000000; ++attempt) {
        if ((inb(COM1_PORT + 5) & 0x20) != 0) {
            outb(COM1_PORT, (uint8_t)character);
            return;
        }
    }
}

void serial_write(const char *text)
{
    if (text == 0) {
        return;
    }

    while (*text != '\0') {
        if (*text == '\n') {
            serial_write_char('\r');
        }

        serial_write_char(*text);
        ++text;
    }
}

void serial_write_line(const char *text)
{
    serial_write(text);
    serial_write("\n");
}

void serial_write_hex(uint64_t value)
{
    static const char digits[] = "0123456789ABCDEF";
    char buffer[16];

    for (uint32_t index = 0; index < 16; ++index) {
        const uint32_t shift = (15U - index) * 4U;
        buffer[index] = digits[(value >> shift) & 0x0FU];
    }

    serial_write("0x");
    for (uint32_t index = 0; index < 16; ++index) {
        serial_write_char(buffer[index]);
    }
}

void serial_write_u64(uint64_t value)
{
    char buffer[20];
    uint32_t length = 0;

    if (value == 0) {
        serial_write_char('0');
        return;
    }

    while (value != 0 && length < sizeof(buffer)) {
        buffer[length++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (length > 0) {
        serial_write_char(buffer[--length]);
    }
}
