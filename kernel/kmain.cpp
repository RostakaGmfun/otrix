#include "kernel/kthread.hpp"
#include "arch/idt.hpp"
#include "otrix/immediate_console.hpp"
#include "dev/acpi.hpp"
#include "dev/lapic.hpp"
#include "dev/serial.h"
#include "dev/pci.h"
#include "arch/paging.hpp"
#include "arch/multiboot2.h"
#include <cstring>
#include <cstdio>

#define VIRTIO_VENDOR_ID 0x1AF4
#define VIRTIO_DEVICE_MIN_ID 0x1000
#define VIRTIO_DEVICE_MAX_ID 0x103F

extern "C" {

static uint64_t t1_stack[1024];
static uint64_t t2_stack[128];

using otrix::immediate_console;
using otrix::kthread;
using otrix::kthread_scheduler;
using otrix::arch::isr_manager;
using otrix::dev::local_apic;

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
                static char buff[128];
                snprintf(buff, sizeof(buff), "Low mem size %d kb, Upper mem start 0x%08x, size %dkb %d\n", mem_low_size, mem_high_start, mem_high_size, tag->size);
                immediate_console::print(buff);
                memset((void *)mem_high_start, 0, mem_high_size * 1024);
                return;
            }
            break;
            default:
                break;

            }
    }
}

__attribute__((noreturn)) void kmain(void)
{
    immediate_console::init();
    isr_manager::load_idt();
    otrix::arch::init_identity_mapping();
    local_apic::init((uint64_t)acpi_get_lapic_addr());
    local_apic::configure_timer(local_apic::timer_mode::oneshot,
            local_apic::timer_divider::divby_2,
            isr_manager::get_systimer_isr_num());
    local_apic::start_timer(1);
    init_heap();

    const int max_devices = 16;
    static pci_device devices[max_devices];
    int num_devices = 0;
    pci_enumerate_devices(devices, max_devices, &num_devices);
    char buff[256];
    for (int i = 0; i < num_devices; i++) {
        snprintf(buff, sizeof(buff), "PCI device %02x:%02x: device_id %x, vendor_id %x, device_class %d, device_subclass %d, subsystem_id %d, subsystem_vendor_id %d\n",
                devices[i].bus_no, devices[i].dev_no, devices[i].device_id, devices[i].vendor_id,
                devices[i].device_class, devices[i].device_subclass, devices[i].subsystem_id,
                devices[i].subsystem_vendor_id);
        immediate_console::print(buff);
        snprintf(buff, sizeof(buff), "    BAR0 0x%08x BAR1 0x%08x BAR2 0x%08x BAR3 0x%08x BAR4 0x%08x BAR5 0x%08x\n",
                devices[i].BAR0, devices[i].BAR1, devices[i].BAR2, devices[i].BAR3, devices[i].BAR4, devices[i].BAR5);
        immediate_console::print(buff);
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

    kthread_scheduler::schedule();

    while (1);
}

}
