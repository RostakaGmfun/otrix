#include <cstring>
#include <cstdio>

#include "kernel/kthread.hpp"
#include "arch/irq_manager.hpp"
#include "arch/asm.h"
#include "arch/pic.hpp"
#include "arch/lapic.hpp"
#include "otrix/immediate_console.hpp"
#include "arch/paging.hpp"
#include "arch/multiboot2.h"
#include "kernel/kmem.h"
#include "arch/kvmclock.hpp"

using otrix::immediate_console;
using otrix::kthread;
using otrix::scheduler;
using otrix::arch::local_apic;

static kmem_heap_t root_heap;

static void init_heap()
{
    extern uint32_t *__multiboot_addr;
    uint64_t addr = (uint64_t)__multiboot_addr & 0xFFFFFFFF;
    struct multiboot_tag *tag;
    for (tag = (struct multiboot_tag *) (addr + 8); tag->type != MULTIBOOT_TAG_TYPE_END; tag = (struct multiboot_tag *) ((multiboot_uint8_t *) tag + ((tag->size + 7) & ~7))) {
        switch (tag->type)
            {
            case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO: {
                const uint64_t mem_low_size = ((struct multiboot_tag_basic_meminfo *) tag)->mem_lower;
                extern uint64_t __binary_end;
                void *mem_high_start = &__binary_end;
                const uint64_t mem_high_size_kb = ((struct multiboot_tag_basic_meminfo *) tag)->mem_upper;
                immediate_console::print("Low mem size %d kb, Upper mem start %p, size %dkb\n", mem_low_size, mem_high_start, mem_high_size_kb);
                // For debugging
                memset(mem_high_start, 0xa5, mem_high_size_kb);
                kmem_init(&root_heap, mem_high_start, mem_high_size_kb * 1024);
                return;
            }
            break;
            default:
                break;

            }
    }
}

namespace otrix {

void *alloc(size_t size)
{
    long flags = arch_irq_save();
    void *ret = kmem_alloc(&root_heap, size);
    arch_irq_restore(flags);
    return ret;
}

void free(void *ptr)
{
    long flags = arch_irq_save();
    kmem_free(&root_heap, ptr);
    arch_irq_restore(flags);
}

extern __attribute__((noreturn)) void otrix_main();

}

extern "C" __attribute__((noreturn)) void kmain(void)
{
    immediate_console::init();
    init_heap();
    otrix::arch::init_identity_mapping();
    otrix::arch::pic_init(32, 40);
    otrix::arch::pic_disable();
    otrix::arch::irq_manager::init();
    local_apic::init(0);
    local_apic::init_timer(otrix::arch::irq_manager::request_irq(otrix::scheduler::handle_timer_irq, "APIC timer"));
    if (!otrix::arch::kvmclock::init()) {
        immediate_console::print("Failed to initialize KVMclock\n");
    }
    arch_enable_interrupts();
    otrix::otrix_main();
}
