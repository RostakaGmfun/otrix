#include "arch/lapic.hpp"

namespace otrix::arch
{

enum lapic_registers
{
    lapic_id_reg = 0x20,
    lapic_version_reg = 0x30,
    lapic_eoi_reg = 0xB0,
    lapic_timer_lvt = 0x320,
    lapic_timer_divider_cfg = 0x3E0,
    lapic_timer_initial_cnt = 0x380,
    lapic_timer_current_cnt = 0x390,
};

volatile uint32_t *local_apic::lapic_ptr_;

uint32_t local_apic::read32(const uint64_t reg)
{
    return *(lapic_ptr_ + reg);
}

void local_apic::write32(const uint64_t reg, const uint32_t value)
{
    *(lapic_ptr_ + reg) = value;
}

void local_apic::init(const uint64_t base_address)
{
    lapic_ptr_ = reinterpret_cast<volatile uint32_t *>(base_address);
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

} // namespace otrix::arch
