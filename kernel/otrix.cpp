#include "net/net_task.hpp"
#include "kernel/kthread.hpp"

namespace otrix
{

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

void otrix_main()
{
    otrix::dev::pci_dev::find_devices(pci_dev_list, sizeof(pci_dev_list) / sizeof(pci_dev_list[0]));
    otrix::net::net_task_start(pci_dev_list[PCI_DEVICE_VIRTIO_NET].p_dev);

    while (1) {
        scheduler::get().schedule();
    }
}

} // namespace otrix
