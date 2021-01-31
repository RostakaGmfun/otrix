#ifndef OTRIX_ARCH_IDT_HPP
#define OTRIX_ARCH_IDT_HPP

#include <cstdint>

namespace otrix::arch
{

struct idt
{
    uint16_t offset_low; //!< Bits 0-15
    uint16_t cs; //!< Code segment of ISR handler
    uint8_t ist; //!< Interrupt stack table
    uint8_t type_dpl; //!< Folks say should be 0x8E
    uint16_t offset_mid; //!< Bits 16-31
    uint32_t offset_high; //!< Bits 32-63
    uint32_t reserved;
} __attribute__((packed));

enum class isr_type
{
    divide_error,
    debug,
    nmi,
    breakpoint,
    overflow,
    bound_range_exceeded,
    invalid_opcode,
    device_not_available,
    double_fault,
    coprocessor_segment_overrun,
    invalid_tss,
    segment_not_present,
    stack_fault,
    general_protection,
    page_fault,
    reserved_exception15,
    fpu_fp_error,
    alignment_check,
    machine_check,
    simd_fp_exception,
    virtualization_exception,
};

class isr_manager
{
public:
    isr_manager() = delete;

    static void load_idt();

    static void generic_handler(const enum isr_type);

    static void nmi_handler();

    static void general_protection_handler();

    static void systimer_interrupt();

    static uint8_t get_systimer_isr_num()
    {
        return systimer_isr_num_;
    }

private:
    static const uint8_t systimer_isr_num_ = 32;
};

} // namespace otrix::arch

#endif // OTRIX_ARCH_IDT_HPP
