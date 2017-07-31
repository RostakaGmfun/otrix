#include "serial.h"
#include "pci.h"

#define VIRTIO_VENDOR_ID 0x1AF4
#define VIRTIO_DEVICE_MIN_ID 0x1000
#define VIRTIO_DEVICE_MAX_ID 0x103F

static const struct
{
    const char *name;
    uint16_t subsystem_id;
} virtio_pci_devices[] = {
    {
        .name = "VirtIO Net",
        .subsystem_id = 0x01,
    },
    {
        .name = "VirtIO Block",
        .subsystem_id = 2,
    },
    {
        .name = "VirtIO Console",
        .subsystem_id = 3,
    },

};

static struct serial_dev serial;

static size_t strlen(const char *str)
{
    size_t ret = 0;
    while (*str++) ret++;
    return ret;
}

static void print(const char *str)
{
    serial_write(&serial, str, strlen(str));
}

__attribute__((noreturn)) void kmain()
{
    serial_init(&serial, SERIAL_BAUD_115200, SERIAL_COM1);
    print("Hello, otrix!\n");

    struct pci_device dev;
    for (uint16_t i = VIRTIO_DEVICE_MIN_ID;
         i < VIRTIO_DEVICE_MAX_ID; i++) {
        if (pci_find_device(i, VIRTIO_VENDOR_ID, &dev) != ENODEV) {
            if (dev.subsystem_id <= 3) {
                print(virtio_pci_devices[dev.subsystem_id].name);
            }
        }
    }
    while (1);
}
