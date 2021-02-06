#include "arch/lapic.hpp"
#include "otrix/immediate_console.hpp"
#include "arch/asm.h"

#define IA32_APIC_BASE_MSR 0x1B
#define IA32_APIC_BASE_MSR_ENABLE 0x800
#define IA32_APIC_BASE_MSR_BSP 0x100
#define IA32_PAT 0x277

namespace otrix::arch
{

enum lapic_registers
{
    lapic_id_reg = 0x20,
    lapic_version_reg = 0x30,
    lapic_eoi_reg = 0xB0,
    lapic_spurious_interrupt = 0xF0,
    lapic_timer_lvt = 0x320,
    lapic_timer_divider_cfg = 0x3E0,
    lapic_timer_initial_cnt = 0x380,
    lapic_timer_current_cnt = 0x390,
};

volatile uint32_t *local_apic::lapic_ptr_;

uint32_t local_apic::read32(const uint64_t reg)
{
    return *((const volatile uint8_t *)lapic_ptr_ + reg);
}

void local_apic::write32(const uint64_t reg, const uint32_t value)
{
    *((volatile uint8_t *)lapic_ptr_ + reg) = value;
}

void local_apic::init(void *base_address)
{
    if (0 == base_address) {
        const uint64_t apic_msr = arch_read_msr(IA32_APIC_BASE_MSR);
        lapic_ptr_ = reinterpret_cast<volatile uint32_t *>(apic_msr & 0xfffff000);
    } else {
        lapic_ptr_ = reinterpret_cast<volatile uint32_t *>(base_address);
    }
    const uint64_t apic_msr_enable = ((uint64_t)lapic_ptr_ & 0xfffff100) | IA32_APIC_BASE_MSR_ENABLE | IA32_APIC_BASE_MSR_BSP;
    arch_write_msr(IA32_APIC_BASE_MSR, apic_msr_enable);
    uint64_t verify = arch_read_msr(IA32_APIC_BASE_MSR);
    for (uint32_t reg = 0x200; reg < 0x210; reg++) {
        const uint64_t msr = arch_read_msr(reg);
        otrix::immediate_console::print("%03x: %016x\n", reg, msr);
    }
    {
        uint32_t reg = 0x250;
        const uint64_t msr = arch_read_msr(reg);
        otrix::immediate_console::print("%03x: %016x\n", reg, msr);
    }
    for (uint32_t reg = 0x258; reg < 0x25a; reg++) {
        const uint64_t msr = arch_read_msr(reg);
        otrix::immediate_console::print("%03x: %016x\n", reg, msr);
    }
    for (uint32_t reg = 0x268; reg < 0x270; reg++) {
        const uint64_t msr = arch_read_msr(reg);
        otrix::immediate_console::print("%03x: %016x\n", reg, msr);
    }
    const uint64_t pat_msr = arch_read_msr(IA32_PAT);
    otrix::immediate_console::print("IA32_PAT: %016x\n", pat_msr);
    if (IA32_APIC_BASE_MSR_ENABLE == (verify & IA32_APIC_BASE_MSR_ENABLE)) {
        otrix::immediate_console::print("xapic ok %016x\n", verify);
    } else {
        otrix::immediate_console::print("xapic fail\n");
    }
    write32(lapic_spurious_interrupt, read32(lapic_spurious_interrupt) | 0x100);
}

int32_t local_apic::id()
{
    return read32(lapic_id_reg);
}

int32_t local_apic::version()
{
    return read32(lapic_version_reg);
}

void local_apic::configure_timer(const enum timer_mode mode,
        const timer_divider divider,
        const uint8_t isr_vector_number)
{
    write32(lapic_timer_divider_cfg, static_cast<uint32_t>(divider));
    write32(lapic_timer_lvt,
            (static_cast<uint32_t>(mode) << 17) | isr_vector_number);
}

void local_apic::start_timer(const int32_t num_cnt)
{
    write32(lapic_timer_initial_cnt, num_cnt);
}

void local_apic::stop_timer()
{
    write32(lapic_timer_initial_cnt, 0);
}

int32_t local_apic::get_timer_counts()
{
    return read32(lapic_timer_current_cnt);
}

uint8_t local_apic::get_active_irq()
{
    // TODO
    return 0;
}

void local_apic::signal_eoi()
{
    write32(lapic_eoi_reg, 0);
}

void local_apic::print_regs()
{
    using otrix::immediate_console;
    immediate_console::print("LAPIC/%d @ %p, version %08x\n", *(volatile uint32_t *)0xfee000b0, lapic_ptr_, read32(lapic_id_reg));
    immediate_console::print("LVT: %08x Spurious vector %08x\n", read32(lapic_timer_lvt), read32(lapic_spurious_interrupt));
}

} // namespace otrix::arch
