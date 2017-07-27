#ifndef OTRIX_PCI_H
#define OTRIX_PCI_H

#include <stdint.h>

struct pci_device
{
    uint32_t pci_config_addr;
    uint16_t device_id;
    uint16_t vendor_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t revision_id;
    uint8_t interrupt_pin;
};

/**
 * Find PCI device by Device ID.
 *
 * @param device_id[in] Device ID to find.
 * @param device[out] Pointer to @pci_device structure to be filled with device info.
 *
 * @retval ENODEV No device found.
 * @retval EOK Success.
 */
error_t pci_find_device(const uint16_t device_id, struct pci_device * const device);

#endif // OTRIX_PCI_H
