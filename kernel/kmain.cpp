#include "kernel/kthread.hpp"
#include "arch/idt.hpp"
#include "otrix/immediate_console.hpp"
// TODO(RostakaGMfun): Provide patch for mini-printf
#include "mini-printf.h"
#undef snprintf
#undef vsnprintf

#define VIRTIO_VENDOR_ID 0x1AF4
#define VIRTIO_DEVICE_MIN_ID 0x1000
#define VIRTIO_DEVICE_MAX_ID 0x103F

/*
static const struct
{
    const char *name;
} virtio_pci_devices[] = {
    {
        .name = "VirtIO Net",
    },
    {
        .name = "VirtIO Block",
    },
    {
        .name = "VirtIO Membaloon",
    },
    {
        .name = "VirtIO Console",
    },

};


static struct kthread t1;
static struct kthread t2;

static void thread1(void *param)
{
    print("thread1 started\n");
    if (*(uint32_t*)param == 42) {
        print("param is 42\n");
    } else {
        print("thread1 BUG ON\n");
    }
    kthread_yield();
    print("thread1 after yielding\n");
    kthread_yield();
    print("thread1 after second yield (thread2 destroyed)\n");
    kthread_yield();
    print("thread1 after 3rd yield (rescheduled as a single thread)\n");
    while (1);
}

static void thread2(void *param)
{
    print("thread2 started\n");
    kthread_yield();
    print("thread2 after yielding\n");
    kthread_destroy(kthread_get_current_id());
    print("BUG ON\n");
}

static void print_virtio_pci(const struct pci_device *dev)
{
    char strbuf[256];
    int dev_id = dev->device_id - VIRTIO_DEVICE_MIN_ID;
    const char *dev_name = "Unknown";
    if (dev_id < sizeof(virtio_pci_devices)/sizeof(virtio_pci_devices[0])) {
        dev_name = virtio_pci_devices[dev_id].name;
    }

    snprintf(strbuf, sizeof(strbuf),
            "Found VirtIO PCI device '%s':\nBus: %d, dev:%d\ndev_id: 0x%X, revision: 0x%X\n",
            dev_name,
            dev->bus_no, dev->dev_no, dev->device_id,
            dev->revision_id);
    print(strbuf);
}
*/

extern "C" {

static uint64_t t1_stack[128];
static uint64_t t2_stack[128];

using otrix::immediate_console;
using otrix::kthread;
using otrix::kthread_scheduler;
using otrix::arch::isr_manager;

__attribute__((noreturn)) void kmain()
{
    immediate_console::init();
    isr_manager::load_idt();

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
                immediate_console::print("after yield\r\n");
                while (1);
            });

    kthread_scheduler::add_thread(thread1);
    kthread_scheduler::add_thread(thread2);

    kthread_scheduler::schedule();
    while (1);
}

}
