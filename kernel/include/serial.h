#ifndef ORBIT_KERNEL_SERIAL_H
#define ORBIT_KERNEL_SERIAL_H

#include <stdint.h>

void serial_init(void);
void serial_write_char(char character);
void serial_write(const char *text);
void serial_write_line(const char *text);
void serial_write_hex(uint64_t value);
void serial_write_u64(uint64_t value);

#endif
