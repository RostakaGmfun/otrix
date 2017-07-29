#include "pci.h"
#include "arch/io.h"

#define PCI_CFG_ADDR 0xCF8
#define PCI_CFG_DATA 0xCFC
#define PCI_CFG_SIZE 0xFF

#define PCI_CFG_ADDR_ENABLE (1 << 31)
#define PCI_CFG_BUS_NUMBER(bus) (((bus) & 0xFF) << 16)
#define PCI_CFG_DEV_NUMBER(dev) (((dev) & 0x1F) << 11)
#define PCI_CFG_FUN_NUMBER(fun) (((fun) & 0x03) << 8)
#define PCI_CFG_REG_NUMBER(reg) (((reg) & 0x3F) << 2)

#define PCI_ADDR_MAX_DEVICES 0x1F
#define PCI_ADDR_MAX_BUSES 0xFF

#define PCI_CFG_REG(bus, dev, fun, reg) PCI_CFG_ADDR_ENABLE | \
    PCI_CFG_DEV_NUMBER(dev) | PCI_CFG_FUN_NUMBER(fun) | PCI_CFG_REG_NUMBER(reg)

enum pci_registers
{
#define PCI_REG_DEVICE_ID(reg) ((reg & 0xFFFF0000) >> 16)
#define PCI_REG_VENDOR_ID(reg) (reg & 0x0000FFFF)
#define PCI_REG_ILLEGAL_VENDOR 0xFFFF
    PCI_REG_DEVICE_VENDOR = 0x00,
#define PCI_REG_STATUS(reg) ((reg & 0xFFFF0000) >> 16)
#define PCI_REG_COMMAND(reg) (reg & 0x0000FFFF)
    PCI_REG_STATUS_COMMAND = 0x04,
#define PCI_REG_CLASS_CODE(reg) ((reg & 0xFF000000) >> 24)
#define PCI_REG_SUBCLASS(reg) ((reg & 0x00FF0000) >> 16)
#define PCI_REG_PROG_IF(reg) ((reg & 0x0000FF00) >> 8)
#define PCI_REG_REVISION_ID(reg) (reg & 0x000000FF)
    PCI_REG_CLASS_SUBCLASS_REV_ID = 0x08,
#define PCI_REG_BIST(reg) ((reg & 0xFF000000) >> 24)
#define PCI_REG_HDR_TYPE(reg) ((reg & 0x00FF0000) >> 16)
    PCI_REG_BIST_HDR_TYPE = 0x0C,
#define PCI_REG_SUBSYSTEM_ID(reg) ((reg & 0xFFFF0000) >> 16)
#define PCI_REG_SUBSYSTEM_VENDOR_ID(reg) (reg & 0x0000FFFF)
    PCI_REG_SUBSYSTEM_ID = 0x2C,
    PCI_REG_BAR0 = 0x10,
    PCI_REG_BAR1 = 0x14,
#define PCI_REG_GET_CAP_PTR(reg) (reg & 0x000000FF)
    PCI_REG_CAP_PTR = 0x34,
#define PCI_REG_INT_PIN(reg) ((reg & 0x0000FF00) >> 8)
#define PCI_REG_INT_LINE(reg) (reg & 0x000000FF)
    PCI_REG_INTERRUPT = 0x3C,
};

#define PCI_CAP_ID 0x00
#define PCI_CAP_NEXT 0x01

#define PCI_REG_STATUS_CAPLIST (1 << 4)

/**
 * Read single 32-bit register from PCI Configuration space
 * for specified bus, device and function.
 */
static inline uint32_t pci_config_read32(const uint8_t bus, const uint8_t dev,
        const uint8_t fun, const uint8_t reg)
{
    arch_io_write32(PCI_CFG_ADDR, PCI_CFG_REG(bus, dev, fun, reg));

    return arch_io_read32(PCI_CFG_DATA);
}

/**
 * Read byte from unaligned address of PCI configuration space.
 */
static inline uint8_t pci_config_read8(const uint8_t bus, const uint8_t dev,
        const uint8_t fun, uint8_t addr)
{
    uint8_t aligned_addr = addr & 0x03;

    uint32_t data = pci_config_read32(bus, dev, fun, aligned_addr);
    return (data >> (addr & 0x03)) & 0xFF;
}

/**
 * Read the PCI device capabilities list into the internal\
 * buffer located in @c pci_device structure.
 *
 * @param device[in,out] Pointer to the device structure.
 *
 * @note This funciton expects that all fields except @c capabilities are filled
 *       with correct values.
 */
static error_t pci_read_device_capabilities(struct pci_device * const device)
{
    size_t cap = 0;
    uint8_t next;
    do {
        device->capabilities[cap].id = pci_config_read8(device->bus_no,
                device->dev_no, device->function, device->cap_ptr + PCI_CAP_ID);
        device->capabilities[cap].offset = pci_config_read8(device->bus_no,
                device->dev_no, device->function, device->cap_ptr + PCI_CAP_NEXT);
        cap++;
    } while (next != 0x00);

    return EOK;
}

/**
 * Fill the @c pci_device structure with data from Configuration Space.
 *
 * @param device[in,out] Pointer to the @c pci_device structure to fill.
 *
 * @note This function expects that the bus_no, dev_no, function, device_id, vendor_id
 *       fields have valid values.
 */
static error_t pci_read_device_config(struct pci_device * const device)
{
    uint32_t reg = pci_config_read32(device->bus_no, device->dev_no,
            device->function, PCI_REG_CLASS_SUBCLASS_REV_ID);
    device->class = PCI_REG_CLASS_CODE(reg);
    device->subclass = PCI_REG_SUBCLASS(reg);
    device->revision_id = PCI_REG_REVISION_ID(reg);

    reg = pci_config_read32(device->bus_no, device->dev_no, device->function, PCI_REG_BAR0);
    device->BAR0 = ((uint64_t)reg << 32) | pci_config_read32(device->bus_no, device->dev_no,
            device->function, PCI_REG_BAR1);

    reg = pci_config_read32(device->bus_no, device->dev_no,
                    device->function, PCI_REG_STATUS_COMMAND);
    device->status = PCI_REG_STATUS(reg);
    device->command = PCI_REG_COMMAND(reg);

    if (device->status & PCI_REG_STATUS_CAPLIST) {
        device->cap_ptr = PCI_REG_GET_CAP_PTR(pci_config_read32(device->bus_no, device->dev_no,
                        device->function, PCI_REG_CAP_PTR));

        return pci_read_device_capabilities(device);
    } else {
        device->cap_ptr = 0x00;
    }

    return EOK;
}


error_t pci_find_device(const uint16_t device_id, const uint16_t vendor_id,
        struct pci_device * const device)
{
    for (int bus = 0; bus < PCI_ADDR_MAX_BUSES; bus++) {
        for (int dev = 0; dev < PCI_ADDR_MAX_DEVICES; dev++) {
            uint32_t dev_vend = pci_config_read32(bus, dev, 0, PCI_REG_DEVICE_VENDOR);
            if (PCI_REG_VENDOR_ID(dev_vend) == PCI_REG_ILLEGAL_VENDOR) {
                continue;
            }

            if (device_id == PCI_REG_DEVICE_ID(dev_vend)
                    &&vendor_id == PCI_REG_VENDOR_ID(dev_vend)) {
                device->bus_no = bus;
                device->dev_no = dev;
                device->vendor_id = vendor_id;
                device->device_id = device_id;
                device->function = 0; // TODO(RostakaGmfun): handle device functions
                return pci_read_device_config(device);
            }
        }
    }

    return ENODEV;
}

error_t pci_read_capability(const struct pci_device * const device,
        const uint8_t cap_id, uint8_t * const buffer, const size_t size)
{
    // TODO(RostakaGmfun)
    return EOK;
}