#ifndef OTRIX_PCI_H
#define OTRIX_PCI_H

#include "common/error.h"
#include <stdint.h>
#include <stddef.h>

#define PCI_CFG_NUM_CAPABILITIES 16

struct pci_capability
{
    uint8_t id;
    uint8_t offset;
};

struct pci_device
{
    uint8_t bus_no;
    uint8_t dev_no;
    uint8_t function;
    uint16_t device_id;
    uint16_t vendor_id;
    uint16_t status;
    uint16_t command;
    uint8_t class;
    uint8_t subclass;
    uint8_t revision_id;
    uint16_t subsystem_id;
    uint16_t subsystem_vendor_id;
    uint64_t BAR0;
    uint8_t cap_ptr;
    struct pci_capability capabilities[PCI_CFG_NUM_CAPABILITIES];
};

/**
 * Find PCI device by Device ID and Vendor ID.
 *
 * @param device_id[in] Device ID of PCI device to find.
 * @param device_id[in] Vendor ID of PCI device to find.
 * @param device[out] Pointer to @pci_device structure to be filled with device info.
 *
 * @retval ENODEV No device found.
 * @retval EOK Success.
 */
error_t pci_find_device(const uint16_t device_id, const uint16_t vendor_id,
        struct pci_device * const device);

/**
 * Read capability data into buffer.
 *
 * @param device[in] PCI device to read the capability for.
 * @param cap_id[in] Device capability to read.
 * @param buffer[out] Buffer to write capability data to.
 * @param size[in] Number of bytes to read (starting right after the @c next pointer).
 *
 * @retval EINVAL Capability no present in given device.
 */
error_t pci_read_capability(const struct pci_device * const device,
        const uint8_t cap_id, uint8_t * const buffer, const size_t size);

#endif // OTRIX_PCI_H
