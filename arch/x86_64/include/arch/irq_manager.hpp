#pragma once

#include <cstdint>

extern "C" void irq_manager_irq_handler();

namespace otrix::arch
{

enum class exception_type
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

// Forward declaration
void irq_manager_irq_manager();

using irq_handler_t = void (*)();

class irq_manager
{
public:
    irq_manager() = delete;

    static void init();

    static int request_irq(irq_handler_t p_handler, const char *p_owner);

    static void free_irq(int irq);

    static void print_irq();

private:

    struct idt_pointer
    {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed));

    static void set_entry(irq_handler_t handler, const uint8_t entry_num);
    static idt_pointer generate_idt();

    static constexpr auto NUM_IRQ = 256;
    static constexpr auto FIRST_USER_IRQ_NUM = 32;

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

    alignas(16) static idt idt_table[irq_manager::NUM_IRQ];

    struct irq_entry {
        bool allocated;
        unsigned int counter;
        const char *p_owner;
        irq_handler_t handler;
    };

    static irq_entry irq_table[NUM_IRQ];

    static void irq_handler();

    friend void ::irq_manager_irq_handler();
};

} // namespace otrix::arch
