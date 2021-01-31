#include <arch/idt.hpp>

#include <cstdint>
#include <otrix/immediate_console.hpp>
#include <arch/io.h>

namespace otrix::arch
{

static const char *exception_names[] = {
    "divide_error",
    "debug",
    "nmi",
    "breakpoint",
    "overflow",
    "bound_range_exceeded",
    "invalid_opcode",
    "device_not_available",
    "double_fault",
    "coprocessor_segment_overrun",
    "invalid_tss",
    "segment_not_present",
    "stack_fault",
    "general_protection",
    "page_fault",
    "reserved_exception15",
    "fpu_fp_error",
    "alignment_check",
    "machine_check",
    "simd_fp_exception",
    "virtualization_exception",
};
void isr_manager::generic_handler(const enum isr_type type)
{
    immediate_console::print(exception_names[static_cast<int>(type)]);
    immediate_console::print("\r\n");
    while (1);
}

void isr_manager::general_protection_handler()
{
    while (1);
}

void isr_manager::nmi_handler()
{
    while (1);
}

void isr_manager::systimer_interrupt()
{
    immediate_console::print("Systimer interrupt\r\n");
}

namespace __idt_detail
{

using isr_handler_type =  void (*)();

static void (*isr_handlers[])() = {
    [] {
        isr_manager::generic_handler(isr_type::divide_error);
    },

    [] {
        isr_manager::generic_handler(isr_type::debug);
    },

    [] {
        isr_manager::nmi_handler();
    },

    [] {
        isr_manager::generic_handler(isr_type::breakpoint);
    },

    [] {
        isr_manager::generic_handler(isr_type::overflow);
    },

    [] {
        isr_manager::generic_handler(isr_type::bound_range_exceeded);
    },

    [] {
        isr_manager::generic_handler(isr_type::invalid_opcode);
    },

    [] {
        isr_manager::generic_handler(isr_type::device_not_available);
    },

    [] {
        isr_manager::generic_handler(isr_type::double_fault);
    },

    [] {
        isr_manager::generic_handler(isr_type::coprocessor_segment_overrun);
    },

    [] {
        isr_manager::generic_handler(isr_type::invalid_tss);
    },

    [] {
        isr_manager::generic_handler(isr_type::segment_not_present);
    },

    [] {
        isr_manager::generic_handler(isr_type::stack_fault);
    },

    [] {
        isr_manager::general_protection_handler();
    },

    [] {
        isr_manager::generic_handler(isr_type::page_fault);
    },

    [] {
        isr_manager::generic_handler(isr_type::reserved_exception15);
    },

    [] {
        isr_manager::generic_handler(isr_type::fpu_fp_error);
    },

    [] {
        isr_manager::generic_handler(isr_type::alignment_check);
    },

    [] {
        isr_manager::generic_handler(isr_type::machine_check);
    },

    [] {
        isr_manager::generic_handler(isr_type::simd_fp_exception);
    },

    [] {
        isr_manager::generic_handler(isr_type::virtualization_exception);
    },
};

constexpr auto NUM_IDT_ENTRIES = 256;
alignas(16) static idt idt_table[NUM_IDT_ENTRIES] = { 0 };

struct idt_pointer
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static void set_entry(isr_handler_type handler,
        const uint8_t entry_num)
{
    const uint64_t handler_addr = reinterpret_cast<uint64_t>(handler);
    idt_table[entry_num].offset_low = handler_addr & 0xFFFF;
    idt_table[entry_num].cs = 0x08; // Entry 1 (code) in GDT
    idt_table[entry_num].ist = 0;
    idt_table[entry_num].type_dpl = 0x8E; // interrupt gate, dpl 0
    idt_table[entry_num].offset_mid = handler_addr >> 16;
    idt_table[entry_num].offset_high = handler_addr >> 32;
    idt_table[entry_num].reserved = 0;
}

void default_handler()
{
    while (1);
};

static idt_pointer generate_idt()
{
    for (int i = 0; i < NUM_IDT_ENTRIES; i++) {
        set_entry(default_handler, i);
    }

    const uint64_t idt_addr = reinterpret_cast<uint64_t>(idt_table);
    return { sizeof(idt_table) - 1, idt_addr };
}

} // namespace __idt_detail

otrix::arch::__idt_detail::idt_pointer idt_table_ptr;

void isr_manager::load_idt()
{
    idt_table_ptr = __idt_detail::generate_idt();
    __idt_detail::set_entry(systimer_interrupt, systimer_isr_num_);

    // Set IDT
    asm volatile("lidt %0" : : "m" (idt_table_ptr) : "memory");

    // Disable PIC
    arch_io_write8(0x21, 0xff);
    arch_io_write8(0xa1, 0xff);

    // Enable interrupts
    asm volatile("sti": : :"memory");
}

} // namespace otrix::arch
