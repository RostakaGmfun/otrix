#include "kernel/kthread.hpp"
#include "arch/irq_manager.hpp"
#include "arch/asm.h"
#include "arch/pic.hpp"
#include "arch/lapic.hpp"
#include "otrix/immediate_console.hpp"
#include "dev/acpi.hpp"
#include "dev/serial.h"
#include "dev/pci.h"
#include "arch/paging.hpp"
#include "arch/multiboot2.h"
#include <cstring>
#include <cstdio>

#define VIRTIO_VENDOR_ID 0x1AF4
#define VIRTIO_DEVICE_MIN_ID 0x1000
#define VIRTIO_DEVICE_MAX_ID 0x103F

static uint64_t t1_stack[1024];
static uint64_t t2_stack[1024];

using otrix::immediate_console;
using otrix::kthread;
using otrix::kthread_scheduler;
using otrix::arch::local_apic;

static pci_descriptor_t pci_devices[] = {
    /*
    {
        .name = "virtio-serial",
        .vendor_id = VIRTIO_PCI_VENDOR_ID,
        .device_id = VIRTIO_SERIAL_PCI_DEVICE_ID,
        .prove_fn = virtio_serial_pci_probe,
    },
    */
};

static void init_heap()
{
    extern uint32_t *__multiboot_addr;
    uint64_t addr = (uint64_t)__multiboot_addr & 0xFFFFFFFF;
    uint64_t size = *(uint64_t *) addr;
    struct multiboot_tag *tag;
    for (tag = (struct multiboot_tag *) (addr + 8); tag->type != MULTIBOOT_TAG_TYPE_END; tag = (struct multiboot_tag *) ((multiboot_uint8_t *) tag + ((tag->size + 7) & ~7))) {
        switch (tag->type)
            {
            case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO: {
                const uint64_t mem_low_start = 0;
                const uint64_t mem_low_size = ((struct multiboot_tag_basic_meminfo *) tag)->mem_lower;
                extern uint64_t __binary_end;
                const uint64_t mem_high_start = (uint64_t)&__binary_end;
                const uint64_t mem_high_size = ((struct multiboot_tag_basic_meminfo *) tag)->mem_upper;
                immediate_console::print("Low mem size %d kb, Upper mem start 0x%08x, size %dkb %d\n", mem_low_size, mem_high_start, mem_high_size, tag->size);
                return;
            }
            break;
            default:
                break;

            }
    }
}

extern "C" __attribute__((noreturn)) void kmain(void)
{
    immediate_console::init();
    init_heap();
    otrix::arch::init_identity_mapping();
    if (EOK != acpi_init()) {
        immediate_console::print("Failed to parse ACPI tables\n");
    }
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
    static pci_device devices[max_devices];
    int num_devices = 0;
    pci_enumerate_devices(devices, max_devices, &num_devices);
    for (int i = 0; i < num_devices; i++) {
        immediate_console::print("PCI device %02x:%02x: device_id %x, vendor_id %x, device_class %d, device_subclass %d, subsystem_id %d, subsystem_vendor_id %d\n",
                devices[i].bus_no, devices[i].dev_no, devices[i].device_id, devices[i].vendor_id,
                devices[i].device_class, devices[i].device_subclass, devices[i].subsystem_id,
                devices[i].subsystem_vendor_id);
        immediate_console::print("    BAR0 0x%08x BAR1 0x%08x BAR2 0x%08x BAR3 0x%08x BAR4 0x%08x BAR5 0x%08x\n",
                devices[i].BAR0, devices[i].BAR1, devices[i].BAR2, devices[i].BAR3, devices[i].BAR4, devices[i].BAR5);
    }

    pci_probe(pci_devices, sizeof(pci_devices) / sizeof(pci_devices[0]));

    auto thread1 = kthread(t1_stack, sizeof(t1_stack),
            [](void *a) {
                immediate_console::print("Task1\n");
                kthread_scheduler::get_current_thread().yield();
                while (1);
            });

    auto thread2 = kthread(t2_stack, sizeof(t2_stack),
            [](void *a) {
                immediate_console::print("Task2\n");
                kthread_scheduler::get_current_thread().yield();
                immediate_console::print("after yield\n");
                while (1);
            });

    immediate_console::print("Adding thread 1\n");
    kthread_scheduler::add_thread(thread1);
    immediate_console::print("Added thread 1\n");
    kthread_scheduler::add_thread(thread2);
    immediate_console::print("Added thread 2\n");

    //kthread_scheduler::schedule();
    otrix::arch::irq_manager::print_irq();
    while (1);
}
