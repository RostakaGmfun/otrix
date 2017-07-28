#include "pci.h"
#include "arch/io.h"

#define PCI_CFG_ADDR 0xCF8
#define PCI_CFG_DATA 0xCFC
#define PCI_CFG_SIZE 0xFF

#define PCI_CFC_ADDR_ENABLE (1 << 31)
#define PCI_CFG_BUS_NUMBER(bus) (((bus) & 0xFF) << 16)
#define PCI_CFC_DEV_NUMBER(dev) (((dev) & 0x0F) << 11)
#define PCI_CFC_FUN_NUMBER(fun) (((fun) & 0x03) << 8)
#define PCI_CFC_REG_NUMBER(reg) (((reg) & 0x3F) << 2)

#define PCI_CFG_REG(bus, dev, fun, reg) PCI_CFG_ADDR_ENABLE | \
    PCI_CFG_DEV_NUMBER(dev) | PCI_CFG_FUN_NUMBER(fun) | PCI_CFG_REG_NUMBER(reg)

enum pci_registers
{
#define PCI_REG_DEVICE_ID 0xFFFF0000
#define PCI_REG_VENDOR_ID 0x0000FFFF
    PCI_REG_DEVICE_VENDOR = 0x00,
#define PCI_REG_STATUS 0xFFFF0000
#define PCI_REG_COMMAND 0x0000FFFF
    PCI_REG_STATUS_COMMAND = 0x04,
#define PCI_REG_CLASS_CODE 0xFF000000
#define PCI_REG_SUBCLASS 0x00FF0000
#define PCI_REG_PROG_IF 0x0000FF00
#define PCI_REG_REVISION_ID 0x000000FF
    PCI_REG_CLASS_SUBCLASS = 0x08,
#define PCI_REG_BIST 0xFF000000
#define PCI_REG_HDR_TYPE 0x00FF0000
    PCI_REG_BIST_HDR_TYPE = 0x0C,
#define PCI_REG_SUBSYSTEM_ID 0xFFFF0000
#define PCI_REG_SUBSYSTEM_VENDOR_ID 0x0000FFFF
    PCI_REG_SUBSYSTEM_ID = 0x2C,
    PCI_REG_BAR0 = 0x10,
#define PCI_REG_CAP_PTR 0x000000FF
    PCI_REG_CAP_PTR = 0x34,
#define PCI_REG_INT_PIN 0x0000FF00
#define PCI_REG_INT_LINE 0x000000FF
    PCI_REG_INTERRUPT = 0x3C,
};


static inline uint32_t pci_config_read32(const uint8_t bus, const uint8_t dev,
        const uint8_t fun, const uint8_t reg)
{
    arch_io_write32(PCI_CFG_ADDR, PCI_CFG_REG(bus, dev, fun, reg));

    return arch_io_read32(PCI_CFG_DATA);
}

error_t pci_find_device(const uint16_t device_id, const uint16_t vendor_id,
        struct pci_device * const device)
{

}
