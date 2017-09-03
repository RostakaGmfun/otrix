#include "dev/serial.h"
//#include "dev/pci.h"
//#include "kernel/kthread.h"
#include "kernel/kthread.hpp"
// TODO(RostakaGMfun): Provide patch for mini-printf
#include "mini-printf.h"
#undef snprintf
#undef vsnprintf
//#include "kernel/kthread.hpp"

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

static size_t strlen(const char *str)
{
    size_t ret = 0;
    while (*str++) ret++;
    return ret;
}

static serial_dev serial;

void print(const char *str)
{
    serial_write(&serial, (const uint8_t *)str, strlen(str));
}

extern "C" {

uint64_t t1_stack[128];
uint64_t t2_stack[128];

__attribute__((noreturn)) void kmain()
{
    serial_init(&serial, SERIAL_BAUD_115200, SERIAL_COM1);
    print("Hello, otrix!\n");

    const auto task1 =
    const auto task2 = [](void *a) {
        print("Task2\n");
        while (1);
    };

    using otrix::kthread;
    using otrix::kthread_scheduler;

    auto thread1 = kthread(t1_stack, sizeof(t1_stack),
            [](void *a) {
                print("Task1\n");
                while (1);
            });

    auto thread1 = kthread(t1_stack, sizeof(t1_stack),
            [](void *a) {
                print("Task1\n");
                while (1);
            });

    kthread_scheduler::add_thread(thread1);

    kthread_scheduler::add_thread(kthread(t2_stack, sizeof(t2_stack)));

    kthread_scheduler::schedule();
    /*

    struct pci_device dev;
    for (uint16_t i = VIRTIO_DEVICE_MIN_ID;
         i < VIRTIO_DEVICE_MAX_ID; i++) {
        if (pci_find_device(i, VIRTIO_VENDOR_ID, &dev) != ENODEV) {
            print_virtio_pci(&dev);
        }
    }

    print("Scan finished\n");

    int p1 = 42;
    kthread_create(&t1, thread1, &p1);

    int p2 = 13;
    kthread_create(&t2, thread2, &p2);

    kthread_scheduler_run();
    */
    while (1);
}

}
