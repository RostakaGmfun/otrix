#pragma once

#include <cstdint>

namespace otrix::arch
{

//!
//! Local Advanced Programmable Interrupt Controller driver.
//!
class local_apic
{
public:

    local_apic() = delete;

    //! Sets the base address for LAPIC
    //! \todo There is one LAPIC per CPU
    //!       so this class should not be static in the future.
    static void init(void *base_address);

    //! Retrieve the value of APIC ID register
    static int32_t id();

    //! Retrieve APIC version
    static int32_t version();

    //! Configure the LAPIC timer.
    //!
    //! \param[in] isr_vector_number IRQ number which is used
    //!                              to service timer interrupts.
    static void init_timer(uint8_t isr_vector_number);

    //! Start previously configured timer.
    //!
    //! \param[in] timeout_us Timeout in microseconds.
    static void start_timer(uint64_t tsc_deadline);

    static void stop_timer();

    //! Retrieve number of timer tick counts since the last shot.
    static int32_t get_timer_counts();

    static int get_active_irq();

    static void signal_eoi();

    static void print_regs();

private:
    static uint32_t read32(const uint64_t reg);
    static void write32(const uint64_t reg, const uint32_t value);

    static volatile uint32_t *lapic_ptr_;
};

} // namesace otrix::arch
