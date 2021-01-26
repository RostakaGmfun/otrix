#ifndef OTRIX_DEV_LAPIC_HPP
#define OTRIX_DEV_LAPIC_HPP

#include <cstdint>

namespace otrix::dev
{

//!
//! Local Advanced Programmable Interrupt Controller driver.
//!
class local_apic
{
public:

    enum class timer_mode
    {
        oneshot,
        periodic,
    };

    enum class timer_divider
    {
        divby_2 = 0x00,
        divby_4 = 0x01,
        divby_32 = 0x8,
    };

    local_apic() = delete;

    //! Sets the base address for LAPIC
    //! \todo There is one LAPIC per CPU
    //!       so this class should not be static in the future.
    static void init(const uint64_t base_address);

    //! Retrieve the value of APIC ID register
    static int32_t id();

    //! Retrieve APIC version
    static int32_t version();

    //! Configure the LAPIC timer.
    //!
    //! \param[in] mode Mode (one-shot/periodic).
    //! \param[in] divider Frequency divider
    //!                    of the bus clock used as a source.
    //! \param[in] isr_vector_number IRQ number which is used
    //!                              to service timer interrupts.
    static void configure_timer(const enum timer_mode mode,
            const timer_divider divider,
            const uint8_t isr_vector_number);

    //! Start previously configured timer.
    //!
    //! \param[in] num_cnt Number of counts to tick.
    static void start_timer(const int32_t num_cnt);

    static void stop_timer();

    //! Retrieve number of timer tick counts since the last shot.
    static int32_t get_timer_counts();

private:
    static uint64_t base_address_;
};

} // namesace otrix::dev

#endif // OTRIX_DEV_LAPIC_HPP