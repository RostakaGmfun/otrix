#ifndef OTRIX_ARCH_IO_H
#define OTRIX_ARCH_IO_H

static inline uint8_t arch_io_read_byte(const uint16_t address)
{
    uint8_t ret;
    asm volatile ("inb %%dx, %%al": "=a" (ret) : "d" (address));
    return ret;
}

static inline void arch_io_write_byte(const uint16_t address, const uint8_t data)
{
    asm volatile ("outb %%al, %%dx": : "d" (address), "a" (data));
}

#endif // OTRIX_ARCH_IO_H
