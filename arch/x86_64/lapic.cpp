#include "arch/lapic.hpp"
#include "otrix/immediate_console.hpp"
#include "arch/asm.h"

#define IA32_APIC_BASE_MSR 0x1B
#define IA32_APIC_BASE_MSR_ENABLE 0x800
#define IA32_APIC_BASE_MSR_X2APIC_ENABLE 0x400
#define IA32_APIC_BASE_MSR_BSP 0x100
#define IA32_PAT 0x277
#define IA32_MSR_X2APIC_MMIO 0x800
#define IA32_MSR_TSC_DEADLINE 0x6e0

namespace otrix::arch
{

enum lapic_registers
{
    lapic_id_reg = 0x02,
    lapic_version_reg = 0x03,
    lapic_eoi_reg = 0x0B,
    lapic_spurious_interrupt = 0x0F,
    lapic_isr = 0x10,
    lapic_timer_lvt = 0x32,
    lapic_timer_divider_cfg = 0x3E,
    lapic_timer_initial_cnt = 0x38,
    lapic_timer_current_cnt = 0x39,
};

volatile uint32_t *local_apic::lapic_ptr_;

uint32_t local_apic::read32(const uint64_t reg)
{
    return arch_read_msr(IA32_MSR_X2APIC_MMIO + reg);
}

void local_apic::write32(const uint64_t reg, const uint32_t value)
{
    arch_write_msr(IA32_MSR_X2APIC_MMIO + reg, value);
}

void local_apic::init(void *base_address)
{
    if (0 == base_address) {
        const uint64_t apic_msr = arch_read_msr(IA32_APIC_BASE_MSR);
        lapic_ptr_ = reinterpret_cast<volatile uint32_t *>(apic_msr & 0xfffff000);
    } else {
        lapic_ptr_ = reinterpret_cast<volatile uint32_t *>(base_address);
    }
    const uint64_t apic_msr_enable = ((uint64_t)lapic_ptr_ & 0xfffff100) | IA32_APIC_BASE_MSR_ENABLE | IA32_APIC_BASE_MSR_BSP | IA32_APIC_BASE_MSR_X2APIC_ENABLE;
    arch_write_msr(IA32_APIC_BASE_MSR, apic_msr_enable);

    uint64_t verify = arch_read_msr(IA32_APIC_BASE_MSR);
    if (IA32_APIC_BASE_MSR_ENABLE == (verify & IA32_APIC_BASE_MSR_ENABLE)) {
        otrix::immediate_console::print("x2apic ok %016x\n", verify);
    } else {
        otrix::immediate_console::print("x2apic fail\n");
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

void local_apic::init_timer(const uint8_t isr_vector_number)
{
    arch_write_msr(IA32_MSR_TSC_DEADLINE, 0);
    write32(lapic_timer_lvt, (2 << 17) | isr_vector_number);
}

void local_apic::start_timer(uint64_t tsc_deadline)
{
    arch_write_msr(IA32_MSR_TSC_DEADLINE, tsc_deadline);
}

void local_apic::stop_timer()
{
    arch_write_msr(IA32_MSR_TSC_DEADLINE, 0);
}

int32_t local_apic::get_timer_counts()
{
    return read32(lapic_timer_current_cnt);
}

int local_apic::get_active_irq()
{
    for (int i = 8; i >= 1; i--) {
        const uint32_t isr = read32(lapic_isr + i);
        if (isr) {
            return 32 * i + __builtin_ffs(isr) - 1;
        }
    }
    return -1;
}

void local_apic::signal_eoi()
{
    write32(lapic_eoi_reg, 0);
}

void local_apic::print_regs()
{
    using otrix::immediate_console;
    immediate_console::print("LAPIC/%d, version %08x\n", read32(lapic_id_reg), read32(lapic_version_reg));
    immediate_console::print("LVT: %08x Spurious vector %08x\n", read32(lapic_timer_lvt), read32(lapic_spurious_interrupt));
}

} // namespace otrix::arch
