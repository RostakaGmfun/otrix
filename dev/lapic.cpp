#include "dev/lapic.hpp"

namespace otrix::dev
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

uint64_t local_apic::base_address_;

static uint32_t read32(const uint64_t addr)
{
    return *reinterpret_cast<volatile uint32_t *>(addr);
}

static uint32_t write32(const uint64_t addr, const uint32_t value)
{
    return *reinterpret_cast<volatile uint32_t *>(addr) = value;
}

void local_apic::init(const uint64_t base_address)
{
    base_address_ = base_address;
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

} // namespace otrix::dev
