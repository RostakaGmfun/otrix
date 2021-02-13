#include "dev/virtio.hpp"
#include "arch/irq_manager.hpp"
#include "otrix/immediate_console.hpp"

/* Common configuration */
#define VIRTIO_PCI_CAP_COMMON_CFG 1
/* Notifications */
#define VIRTIO_PCI_CAP_NOTIFY_CFG 2
/* ISR Status */
#define VIRTIO_PCI_CAP_ISR_CFG    3
/* Device specific configuration */
#define VIRTIO_PCI_CAP_DEVICE_CFG 4
/* PCI configuration access */
#define VIRTIO_PCI_CAP_PCI_CFG    5

#define VIRTIO_PCI_VENDOR_ID 0x1af4

namespace otrix::dev
{

enum virtio_device_status
{
    acknowledge = 1,
    driver = 2,
    failed = 128,
    features_ok = 8,
    driver_ok = 4,
    needs_reset = 64,
};

using otrix::immediate_console;

virtio_dev::virtio_dev(pci_device *pci_dev): pci_dev_(pci_dev)
{
    if (pci_dev->vendor_id != VIRTIO_PCI_VENDOR_ID) {
        return;
    }
    for (int i = 0; i < PCI_CFG_NUM_CAPABILITIES; i++) {
        if (PCI_CAP_ID_VENDOR == pci_dev->capabilities[i].id) {
            const uint8_t cfg_type = pci_dev_read8(pci_dev, pci_dev->capabilities[i].offset + 3);
            if (cfg_type < 1 || cfg_type > 5) {
                immediate_console::print("Unknown Virtio cfg_type=%d\n", cfg_type);
                continue;
            }

            const uint8_t bar = pci_dev_read8(pci_dev, pci_dev->capabilities[i].offset + 4);
            if (bar > 5) {
                immediate_console::print("Wrong BAR id: %d\n", bar);
                continue;
            }
            caps_[cfg_type - 1].base = pci_dev->BAR[bar];
            caps_[cfg_type - 1].offset = pci_dev_read32(pci_dev, pci_dev->capabilities[i].offset + 8);
            caps_[cfg_type - 1].len = pci_dev_read32(pci_dev, pci_dev->capabilities[i].offset + 12);
        }
    }
    valid_ = true;

    write_reg(device_status, 0);
    write_reg(device_status, virtio_device_status::acknowledge | virtio_device_status::driver);
    write_reg(driver_features, read_reg(device_features));
    immediate_console::print("Device status %02x, features %08x\n", read_reg(device_status), read_reg(device_features));
}

virtio_dev::~virtio_dev()
{

}

void virtio_dev::print_info()
{
    immediate_console::print("Virtio capabilities:\n");
    for (int i = 0; i < 5; i++) {
        immediate_console::print("CAP%d @ %p, len %d\n", i + 1, caps_[i].base + caps_[i].offset, caps_[i].len);
    }
}

uint32_t virtio_dev::read_reg(virtio_device_register reg)
{
    switch (reg) {
    case device_features:
    case driver_features:
    case queue_address:
        return pci_dev_read32(pci_dev_, pci_dev_->BAR[0] + reg);
    case queue_size:
    case queue_select:
    case queue_notify:
        return pci_dev_read16(pci_dev_, pci_dev_->BAR[0] + reg);
    case device_status:
    case isr_status:
        return pci_dev_read8(pci_dev_, pci_dev_->BAR[0] + reg);
    case config_msix_vector:
    case queue_msix_vector:
        return pci_dev_read16(pci_dev_, pci_dev_->BAR[0] + reg);
    default:
        return 0;
    }
}

void virtio_dev::write_reg(virtio_device_register reg, uint32_t value)
{
    switch (reg) {
    case device_features:
    case driver_features:
    case queue_address:
        return pci_dev_write32(pci_dev_, pci_dev_->BAR[0] + reg, value);
    case queue_size:
    case queue_select:
    case queue_notify:
        return pci_dev_write16(pci_dev_, pci_dev_->BAR[0] + reg, value);
    case device_status:
    case isr_status:
        return pci_dev_write8(pci_dev_, pci_dev_->BAR[0] + reg, value);
    case config_msix_vector:
    case queue_msix_vector:
        return pci_dev_write16(pci_dev_, pci_dev_->BAR[0] + reg, value);
    }
}

} // namespace otrix::dev
