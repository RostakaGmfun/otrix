#include "net/net_task.hpp"
#include "kernel/kthread.hpp"
#include "dev/virtio_blk.hpp"

namespace otrix
{

enum pci_devices_t {
    PCI_DEVICE_VIRTIO_NET,
    PCI_DEVICE_VIRTIO_CONSOLE,
    PCI_DEVICE_VIRTIO_BLK,
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
    [PCI_DEVICE_VIRTIO_BLK] = {
        .device_id = 0x1001,
        .vendor_id = 0x1af4,
        .p_dev = nullptr,
    },
};

void otrix_main()
{
    otrix::dev::pci_dev::find_devices(pci_dev_list, sizeof(pci_dev_list) / sizeof(pci_dev_list[0]));

    dev::virtio_blk blk(pci_dev_list[PCI_DEVICE_VIRTIO_BLK].p_dev);
    blk.print_info();

    otrix::net::net_task_start(pci_dev_list[PCI_DEVICE_VIRTIO_NET].p_dev);

    while (1) {
        scheduler::get().schedule();
        asm volatile("hlt");
    }
}

} // namespace otrix
