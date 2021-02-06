#include <arch/irq_manager.hpp>
#include <arch/asm.h>
#include <arch/lapic.hpp>

#include <cstdint>
#include <cstdio>
#include <otrix/immediate_console.hpp>

extern "C" void arch_used_irq_handler();
extern "C" void arch_unused_irq_handler();
extern "C" void arch_exception_handler();

extern "C" void irq_manager_irq_handler()
{
    otrix::arch::irq_manager::irq_handler();
}


namespace otrix::arch
{

alignas(16) irq_manager::idt irq_manager::idt_table[irq_manager::NUM_IRQ];
irq_manager::irq_entry irq_manager::irq_table[irq_manager::NUM_IRQ];

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

void irq_manager::init()
{
    for (int i = 0; i < NUM_IRQ; i++) {
        irq_table[i].allocated = false;
    }

    idt_pointer idt_table_ptr = generate_idt();

    // Set IDT
    asm volatile("lidt %0" : : "m" (idt_table_ptr) : "memory");
}

int irq_manager::request_irq(irq_handler_t p_handler, const char *p_owner)
{
    arch_disable_interrupts();
    int ret = -1;
    for (int i = FIRST_USER_IRQ_NUM; i < NUM_IRQ; i++) {
        if (!irq_table[i].allocated) {
            irq_table[i].allocated = true;
            irq_table[i].p_owner = p_owner;
            irq_table[i].counter = 0;
            irq_table[i].handler = p_handler;
            set_entry(arch_used_irq_handler, i);
            ret = i;
            break;
        }
    }
    arch_enable_interrupts();
    return ret;
}

void irq_manager::free_irq(int irq)
{
    if (irq >= FIRST_USER_IRQ_NUM && irq < NUM_IRQ) {
        irq_table[irq].allocated = false;
    }
}

void irq_manager::print_irq()
{
    char buff[64];
    immediate_console::print("IRQ table:\n");
    for (int i = FIRST_USER_IRQ_NUM; i < NUM_IRQ; i++) {
        if (irq_table[i].allocated) {
            immediate_console::print("IRQ%d called %d times, handler %p %s\n", i, irq_table[i].counter, irq_table[i].handler, irq_table[i].p_owner);
        }
    }
}

void irq_manager::set_entry(irq_handler_t handler,
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

irq_manager::idt_pointer irq_manager::generate_idt()
{
    for (int i = 0; i < FIRST_USER_IRQ_NUM; i++) {
        set_entry(arch_exception_handler, i);
    }

    for (int i = 0; i < NUM_IRQ; i++) {
        set_entry(arch_unused_irq_handler, i);
    }

    const uint64_t idt_addr = reinterpret_cast<uint64_t>(idt_table);
    return { sizeof(idt_table) - 1, idt_addr };
}

void irq_manager::irq_handler()
{
    const uint8_t irq_n = local_apic::get_active_irq();

    if (irq_table[irq_n].allocated) {
        irq_table[irq_n].counter++;
        if (irq_table[irq_n].handler) {
            irq_table[irq_n].handler();
        }
    }

    local_apic::signal_eoi();
}

} // namespace otrix::arch
