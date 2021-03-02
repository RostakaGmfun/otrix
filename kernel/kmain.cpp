#include "kernel/kthread.hpp"
#include "arch/irq_manager.hpp"
#include "arch/asm.h"
#include "arch/pic.hpp"
#include "arch/lapic.hpp"
#include "otrix/immediate_console.hpp"
#include "dev/acpi.hpp"
#include "dev/serial.h"
#include "dev/pci.hpp"
#include "dev/virtio.hpp"
#include "dev/virtio_net.hpp"
#include "dev/virtio_console.hpp"
#include "arch/paging.hpp"
#include "arch/multiboot2.h"
#include "kernel/kmem.h"
#include "kernel/waitq.hpp"
#include "arch/kvmclock.hpp"
#include <cstring>
#include <cstdio>

static uint64_t t1_stack[1024];
static uint64_t t2_stack[1024];

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

}

enum pci_devices_t {
    PCI_DEVICE_VIRTIO_NET,
    PCI_DEVICE_VIRTIO_CONSOLE,
};

static otrix::dev::pci_dev::descriptor_t pci_dev_list[] = {
    [PCI_DEVICE_VIRTIO_NET] = {
        .device_id = 0x1000,
        .vendor_id = 0x1af4,
        .p_dev = nullptr,
    },
    [PCI_DEVICE_VIRTIO_CONSOLE] = {
        .device_id = 0x1003,
        .vendor_id = 0x1af4,
        .p_dev = nullptr,
    },
};

extern "C" __attribute__((noreturn)) void kmain(void)
{
    immediate_console::init();
    // ACPI should be parsed before heap because it is located in usable RAM.
    if (E_OK != acpi_init()) {
        immediate_console::print("Failed to parse ACPI tables\n");
    }
    init_heap();
    otrix::arch::init_identity_mapping();
    otrix::arch::pic_init(32, 40);
    otrix::arch::pic_disable();
    otrix::arch::irq_manager::init();
    local_apic::init(acpi_get_lapic_addr());
    local_apic::print_regs();
    local_apic::init_timer(otrix::arch::irq_manager::request_irq(otrix::scheduler::handle_timer_irq, "APIC timer"));
    if (!otrix::arch::kvmclock::init()) {
        immediate_console::print("Failed to initialize KVMclock\n");
    }
    immediate_console::print("Systime: %lu\n", otrix::arch::kvmclock::systime_ms());
    otrix::arch::irq_manager::print_irq();
    arch_enable_interrupts();

    otrix::dev::pci_dev::find_devices(pci_dev_list, sizeof(pci_dev_list) / sizeof(pci_dev_list[0]));

    if (nullptr != pci_dev_list[PCI_DEVICE_VIRTIO_NET].p_dev) {
        otrix::dev::pci_dev *p_dev = pci_dev_list[PCI_DEVICE_VIRTIO_NET].p_dev;
        otrix::dev::virtio_net virtio_net(p_dev);
        p_dev->print_info();
        virtio_net.print_info();
    }

    if (nullptr != pci_dev_list[PCI_DEVICE_VIRTIO_CONSOLE].p_dev) {
        otrix::dev::pci_dev *p_dev = pci_dev_list[PCI_DEVICE_VIRTIO_CONSOLE].p_dev;
        otrix::dev::virtio_console virtio_console(p_dev);
        virtio_console.print_info();
        p_dev->print_info();
    }

    static otrix::waitq sync;

    auto thread1 = kthread(t1_stack, sizeof(t1_stack),
            []() {
                while (1) {
                    immediate_console::print("Task1\n");
                    immediate_console::print("Task1 after wait, ret = %d\n", sync.wait(1000));
                }
            }, "Task1", 1);

    auto thread2 = kthread(t2_stack, sizeof(t2_stack),
            []() {
                while (1) {
                    immediate_console::print("Task2\n");
                    scheduler::get().sleep(5000);
                    sync.notify_one();
                }
            }, "Task2", 1);

    scheduler::get().add_thread(&thread1);
    immediate_console::print("Added thread 1\n");
    scheduler::get().add_thread(&thread2);
    immediate_console::print("Added thread 2\n");

    immediate_console::print("Systime: %lu\n", otrix::arch::kvmclock::systime_ms());
    while (1) {
        scheduler::get().schedule();
    }
}
