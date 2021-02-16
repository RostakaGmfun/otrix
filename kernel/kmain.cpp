#include "kernel/kthread.hpp"
#include "arch/irq_manager.hpp"
#include "arch/asm.h"
#include "arch/pic.hpp"
#include "arch/lapic.hpp"
#include "otrix/immediate_console.hpp"
#include "dev/acpi.hpp"
#include "dev/serial.h"
#include "dev/pci.h"
#include "dev/virtio.hpp"
#include "dev/virtio_net.hpp"
#include "dev/virtio_console.hpp"
#include "arch/paging.hpp"
#include "arch/multiboot2.h"
#include "kernel/kmem.h"
#include <cstring>
#include <cstdio>

static uint64_t t1_stack[1024];
static uint64_t t2_stack[1024];

using otrix::immediate_console;
using otrix::kthread;
using otrix::kthread_scheduler;
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
                immediate_console::print("Low mem size %d kb, Upper mem start 0x016%p, size %dkb %d\n", mem_low_size, mem_high_start, mem_high_size_kb, tag->size);
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

}

extern "C" __attribute__((noreturn)) void kmain(void)
{
    immediate_console::init();
    // ACPI should be parsed before heap because it is located in usable RAM.
    if (EOK != acpi_init()) {
        immediate_console::print("Failed to parse ACPI tables\n");
    }
    init_heap();
    otrix::arch::init_identity_mapping();
    otrix::arch::pic_init(32, 40);
    otrix::arch::pic_disable();
    otrix::arch::irq_manager::init();
    local_apic::init(acpi_get_lapic_addr());
    local_apic::print_regs();
    local_apic::configure_timer(local_apic::timer_mode::periodic,
            local_apic::timer_divider::divby_32,
            otrix::arch::irq_manager::request_irq(nullptr, "APIC timer"));
    otrix::arch::irq_manager::print_irq();
    arch_enable_interrupts();
    local_apic::start_timer(0x1);

    const int max_devices = 16;
    pci_device *devices = (pci_device *)kmem_alloc(&root_heap, sizeof(pci_device) * max_devices);
    kASSERT(nullptr != devices);
    memset((void *)devices, 0, sizeof(pci_device) * max_devices);
    int num_devices = 0;
    pci_enumerate_devices(devices, max_devices, &num_devices);
    immediate_console::print("Devices enumerated: %d\n", num_devices);
    for (int i = 0; i < num_devices; i++) {
        immediate_console::print("PCI device %02x:%02x: device_id %x, vendor_id %x, device_class %d, device_subclass %d, subsystem_id %d, subsystem_vendor_id %d\n",
                devices[i].bus_no, devices[i].dev_no, devices[i].device_id, devices[i].vendor_id,
                devices[i].device_class, devices[i].device_subclass, devices[i].subsystem_id,
                devices[i].subsystem_vendor_id);
        immediate_console::print("\tBAR0 0x%08x BAR1 0x%08x BAR2 0x%08x BAR3 0x%08x BAR4 0x%08x BAR5 0x%08x\n",
                devices[i].BAR[0], devices[i].BAR[1], devices[i].BAR[2], devices[i].BAR[3], devices[i].BAR[4], devices[i].BAR[5]);
        immediate_console::print("\tCapabilities:\n");
        for (int j = 0; j < PCI_CFG_NUM_CAPABILITIES; j++) {
            if (0 == devices[i].capabilities[j].id) {
                break;
            }
            immediate_console::print("\t\tCAP %02x %02x\n", devices[i].capabilities[j].id, devices[i].capabilities[j].offset);
        }
        if (otrix::dev::virtio_net::is_virtio_net_device(&devices[i])) {
            otrix::dev::virtio_net virtio_net(&devices[i]);
            virtio_net.print_info();
        }
        if (otrix::dev::virtio_console::is_virtio_console_device(&devices[i])) {
            otrix::dev::virtio_console virtio_console(&devices[i]);
            virtio_console.print_info();
        }
    }

    auto thread1 = kthread(t1_stack, sizeof(t1_stack),
            []() {
                immediate_console::print("Task1\n");
                kthread_scheduler::get_current_thread().yield();
                while (1);
            });

    auto thread2 = kthread(t2_stack, sizeof(t2_stack),
            []() {
                immediate_console::print("Task2\n");
                kthread_scheduler::get_current_thread().yield();
                immediate_console::print("after yield\n");
                while (1);
            });

    kthread_scheduler::add_thread(thread1);
    kthread_scheduler::add_thread(thread2);

    otrix::arch::irq_manager::print_irq();
    kthread_scheduler::schedule();
    while (1);
}
