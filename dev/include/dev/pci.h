#ifndef OTRIX_PCI_H
#define OTRIX_PCI_H

#include "common/error.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PCI_CFG_NUM_CAPABILITIES 16

#define PCI_CAP_ID_NULL   0x00
#define PCI_CAP_ID_MSIX   0x11
#define PCI_CAP_ID_VENDOR 0x09

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
    uint8_t device_class;
    uint8_t device_subclass;
    uint8_t revision_id;
    uint16_t subsystem_id;
    uint16_t subsystem_vendor_id;
    uint32_t BAR[6];
    uint8_t cap_ptr;
    struct pci_capability capabilities[PCI_CFG_NUM_CAPABILITIES];
};

typedef struct pci_descriptor_t {
    const char *name;
    struct pci_device *dev;
    error_t (*probe_fn)(struct pci_device *dev);
} pci_descriptor_t;

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

error_t pci_enumerate_devices(struct pci_device *devices, int array_size, int *num_devs_found);

error_t pci_probe(const struct pci_descriptor_t *p_desc, int num_desc);

uint32_t pci_dev_read32(const struct pci_device *device, uint8_t reg);
uint16_t pci_dev_read16(const struct pci_device *device, uint8_t reg);
uint8_t pci_dev_read8(const struct pci_device *device, uint8_t reg);

void pci_dev_write32(const struct pci_device *device, uint8_t reg, uint32_t val);
void pci_dev_write16(const struct pci_device *device, uint8_t reg, uint16_t val);
void pci_dev_write8(const struct pci_device *device, uint8_t reg, uint8_t val);

#ifdef __cplusplus
}
#endif

#endif // OTRIX_PCI_H
