#ifndef OTRIX_ARCH_IO_H
#define OTRIX_ARCH_IO_H

static inline uint8_t arch_io_read8(const uint16_t address)
{
    uint8_t ret;
    asm volatile ("inb %%dx, %%al": "=a" (ret) : "d" (address));
    return ret;
}

static inline void arch_io_write8(const uint16_t address, const uint8_t data)
{
    asm volatile ("outb %%al, %%dx": : "d" (address), "a" (data));
}

static inline uint16_t arch_io_read16(const uint16_t address)
{
    uint16_t ret;
    asm volatile ("inb %%dx, %%ax": "=a" (ret) : "d" (address));
    return ret;
}

static inline void arch_io_write16(const uint16_t address, const uint16_t data)
{
    asm volatile ("outb %%ax, %%dx": : "d" (address), "a" (data));
}

static inline uint16_t arch_io_read32(const uint16_t address)
{
    uint32_t ret;
    asm volatile ("inb %%dx, %%eax": "=a" (ret) : "d" (address));
    return ret;
}

static inline void arch_io_write32(const uint16_t address, const uint32_t data)
{
    asm volatile ("outb %%eax, %%dx": : "d" (address), "a" (data));
}

#endif // OTRIX_ARCH_IO_H
