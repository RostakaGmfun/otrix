#ifndef OTRIX_ARCH_IO_H
#define OTRIX_ARCH_IO_H

#include <stdint.h>

static inline uint8_t arch_io_read8(const uint16_t address)
{
    uint8_t ret;
    asm volatile ("in %%dx, %%al": "=a" (ret) : "d" (address));
    return ret;
}

static inline void arch_io_write8(const uint16_t address, const uint8_t data)
{
    asm volatile ("out %%al, %%dx": : "d" (address), "a" (data));
}

static inline uint16_t arch_io_read16(const uint16_t address)
{
    uint16_t ret;
    asm volatile ("in %%dx, %%ax": "=a" (ret) : "d" (address));
    return ret;
}

static inline void arch_io_write16(const uint16_t address, const uint16_t data)
{
    asm volatile ("out %%ax, %%dx": : "d" (address), "a" (data));
}

static inline uint32_t arch_io_read32(const uint16_t address)
{
    uint32_t ret;
    asm volatile ("in %%dx, %%eax": "=a" (ret) : "d" (address));
    return ret;
}

static inline void arch_io_write32(const uint16_t address, const uint32_t data)
{
    asm volatile ("out %%eax, %%dx": : "d" (address), "a" (data));
}

static inline void arch_io_wait(void)
{
    // Write to dummy IO port to make sure previous IO operation completed
    arch_io_write8(0x80, 0);
}

#endif // OTRIX_ARCH_IO_H
