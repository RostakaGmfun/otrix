#pragma once

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

static inline void arch_disable_interrupts(void)
{
    asm volatile("cli": : :"memory");
}

static inline void arch_enable_interrupts(void)
{
    asm volatile("sti": : :"memory");
}

static inline long arch_irq_save(void)
{
    long flags = 0;
	asm volatile("pushfq ; popq %0;" : "=rm" (flags) : : "memory");
    arch_disable_interrupts();
    return flags;
}

static inline void arch_irq_restore(long flags)
{
	asm volatile("pushq %0 ; popfq" : /* no output */ :"g" (flags) :"memory", "cc");
}

static uint64_t arch_read_msr(uint32_t msr)
{
    uint32_t lo, hi;
    asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

static inline void arch_write_msr(uint32_t msr, uint64_t val)
{
    uint32_t lo = val & 0xFFFFFFFF;
    uint32_t hi = val >> 32;
    asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}
