#include "dev/pci.h"
#include "arch/asm.h"

#define PCI_CFG_ADDR 0xCF8
#define PCI_CFG_DATA 0xCFC
#define PCI_CFG_SIZE 0xFF

#define PCI_CFG_ADDR_ENABLE (1 << 31)
#define PCI_CFG_BUS_NUMBER(bus) (((bus) & 0xFF) << 16)
#define PCI_CFG_DEV_NUMBER(dev) (((dev) & 0x1F) << 11)
#define PCI_CFG_FUN_NUMBER(fun) (((fun) & 0x03) << 8)
#define PCI_CFG_REG_NUMBER(reg) (((reg) & 0xFC))

#define PCI_ADDR_MAX_DEVICES 0x1F
#define PCI_ADDR_MAX_BUSES 0xFF

#define PCI_CFG_REG(bus, dev, fun, reg) PCI_CFG_ADDR_ENABLE | PCI_CFG_BUS_NUMBER(bus) | \
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
    PCI_REG_BAR2 = 0x18,
    PCI_REG_BAR3 = 0x1C,
    PCI_REG_BAR4 = 0x20,
    PCI_REG_BAR5 = 0x24,
#define PCI_REG_SUBSYS_ID(ret) ((reg & 0xFFFF0000) >> 16)
#define PCI_REG_SUBSYS_VENDOR_ID(ret) ((reg & 0x0000FFFF))
    PCI_REG_SUBSYSTEM = 0x2C,
#define PCI_REG_GET_CAP_PTR(reg) (reg & 0x000000FC)
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
    long flags = arch_irq_save();
    arch_io_write32(PCI_CFG_ADDR, PCI_CFG_REG(bus, dev, fun, reg));
    const uint32_t ret = arch_io_read32(PCI_CFG_DATA);
    arch_irq_restore(flags);
    return ret;
}

/**
 * Read 16-bit value from unaligned address of PCI configuration space.
 */
static inline uint16_t pci_config_read16(const uint8_t bus, const uint8_t dev,
        const uint8_t fun, uint8_t addr)
{
    uint8_t aligned_addr = addr & (~0x03);

    const uint32_t data = pci_config_read32(bus, dev, fun, aligned_addr);
    return (data >> ((addr & 0x01) * 16)) & 0xFFFF;
}

/**
 * Read byte from unaligned address of PCI configuration space.
 */
static inline uint8_t pci_config_read8(const uint8_t bus, const uint8_t dev,
        const uint8_t fun, uint8_t addr)
{
    uint8_t aligned_addr = addr & (~0x03);

    const uint32_t data = pci_config_read32(bus, dev, fun, aligned_addr);
    return (data >> ((addr & 0x03) * 8)) & 0xFF;
}

/**
 * Write single 32-bit register from PCI Configuration space
 * for specified bus, device and function.
 */
static inline void pci_config_write32(const uint8_t bus, const uint8_t dev,
        const uint8_t fun, const uint8_t reg, uint32_t val)
{
    long flags = arch_irq_save();
    arch_io_write32(PCI_CFG_ADDR, PCI_CFG_REG(bus, dev, fun, reg));
    arch_io_write32(PCI_CFG_DATA, val);
    arch_irq_restore(flags);
}

/**
 * Write 16-bit value to unaligned address of PCI configuration space.
 */
static inline void pci_config_write16(const uint8_t bus, const uint8_t dev,
        const uint8_t fun, uint8_t addr, uint16_t val)
{
    uint8_t aligned_addr = addr & (~0x03);

    const uint32_t data = pci_config_read32(bus, dev, fun, aligned_addr);
    pci_config_write32(bus, dev, fun, aligned_addr, (data & ~(0xffff << ((addr & 0x01) * 16))) | val << ((addr & 0x01) * 16));
}

/**
 * Write byte from unaligned address of PCI configuration space.
 */
static inline void pci_config_write8(const uint8_t bus, const uint8_t dev,
        const uint8_t fun, uint8_t addr, uint8_t val)
{
    uint8_t aligned_addr = addr & (~0x03);

    const uint32_t data = pci_config_read32(bus, dev, fun, aligned_addr);
    pci_config_write32(bus, dev, fun, aligned_addr, (data & ~(0xff << ((addr & 0x03) * 8))) | val << ((addr & 0x03) * 8));
}

/**
 * Read @c size number of bytes from possibly unaligned address
 * of PCI configuration space.
 *
 * Attempts to perform as much of aligned 4-byte reads as possible.
 *
 * @retval EINVAL Requested read size goes out of bounds
 *                of the 256-byte Configuration space.
 */
static inline error_t pci_config_read_buffer(const uint8_t bus,
        const uint8_t dev, const uint8_t fun, const uint8_t addr,
        uint8_t *buffer, const size_t size)
{
    if (size - addr > 0xFF) {
        return EINVAL;
    }

    uint8_t aligned_addr = addr & (~0x03);
    if (aligned_addr != addr) {
        const uint32_t first_word = pci_config_read32(bus, dev, fun, aligned_addr);
        for (int i = (addr & 0x03); i < 0x03; i++) {
            *buffer++ =  (first_word >> (i * 8)) & 0xFF;
        }
    }

    aligned_addr += 4;

    for (size_t i = 0; i < size / 4; i++) {
        *((uint32_t *)buffer) = pci_config_read32(bus, dev, fun, aligned_addr);
        buffer += 4;
        aligned_addr += 4;
    }

    if (size % 4 != 0) {
        const uint32_t last_word = pci_config_read32(bus, dev, fun, aligned_addr);
        for (size_t i = 0; i < size % 4; i++) {
            *buffer++ = (last_word >> (i * 8)) & 0xFF;
        }

    }
    return EOK;
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
    uint8_t next = device->cap_ptr;
    do {
        device->capabilities[cap].id = pci_config_read8(device->bus_no,
                device->dev_no, device->function, next);
        device->capabilities[cap].offset = next;
        next = pci_config_read8(device->bus_no,
                device->dev_no, device->function, next + 1);
        if (0 != device->capabilities[cap].id) {
            // Don't save NULL capabilities
            cap++;
        }
    } while (next != 0x00 && cap < PCI_CFG_NUM_CAPABILITIES);

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
    device->device_class = PCI_REG_CLASS_CODE(reg);
    device->device_subclass = PCI_REG_SUBCLASS(reg);
    device->revision_id = PCI_REG_REVISION_ID(reg);

    for (int i = 0; i < 6; i++) {
        device->BAR[i] = pci_config_read32(device->bus_no, device->dev_no,
                device->function, PCI_REG_BAR0 + i * 4);
        if (device->BAR[i] & 1) {
            // IO space
            device->BAR[i] &= 0xFFFC;
        }
    }

    reg = pci_config_read32(device->bus_no, device->dev_no,
                    device->function, PCI_REG_STATUS_COMMAND);
    device->status = PCI_REG_STATUS(reg);
    device->command = PCI_REG_COMMAND(reg);

    reg = pci_config_read32(device->bus_no, device->dev_no,
                    device->function, PCI_REG_SUBSYSTEM);
    device->subsystem_id = PCI_REG_SUBSYSTEM_ID(reg);
    device->subsystem_vendor_id = PCI_REG_SUBSYSTEM_VENDOR_ID(reg);

    if (device->status & PCI_REG_STATUS_CAPLIST) {
        device->cap_ptr = PCI_REG_GET_CAP_PTR(pci_config_read32(device->bus_no,
                        device->dev_no, device->function, PCI_REG_CAP_PTR));

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

error_t pci_enumerate_devices(struct pci_device *devices, int array_size, int *num_devs_found) {
    *num_devs_found = 0;
    for (int bus = 0; bus < PCI_ADDR_MAX_BUSES - 1; bus++) {
        for (int dev = 0; dev < PCI_ADDR_MAX_DEVICES - 1; dev++) {
            uint32_t dev_vend = pci_config_read32(bus, dev, 0, PCI_REG_DEVICE_VENDOR);
            if (PCI_REG_VENDOR_ID(dev_vend) == PCI_REG_ILLEGAL_VENDOR) {
                continue;
            }

            struct pci_device *device = devices + *num_devs_found;
            device->bus_no = bus;
            device->dev_no = dev;
            device->vendor_id = PCI_REG_VENDOR_ID(dev_vend);
            device->device_id = PCI_REG_DEVICE_ID(dev_vend);
            device->function = 0; // TODO(RostakaGmfun): handle device functions
            error_t ret = pci_read_device_config(device);
            if (ret != EOK) {
                return ret;
            }
            (*num_devs_found)++;
            if (*num_devs_found == array_size) {
                return EOK;
            }
        }
    }

    return EOK;
}

error_t pci_read_capability(const struct pci_device * const device,
        const uint8_t cap_id, uint8_t * const buffer,
        const size_t size)
{
    if (device == NULL || buffer == NULL || size == 0) {
        return EINVAL;
    }

    const size_t num_caps = sizeof(device->capabilities) /
        sizeof(struct pci_capability);
    for (size_t i = 0; i < num_caps; i++) {
        if (device->capabilities[i].id == cap_id) {
            pci_config_read_buffer(device->bus_no, device->dev_no,
                    device->function,
                    device->capabilities[i].offset + 2, buffer, size);
        }
    }

    return EOK;
}

error_t pci_probe(const struct pci_descriptor_t *p_desc, int num_desc) {
    for (int i = 0; i < num_desc; i++) {
        error_t ret = pci_find_device(p_desc[i].dev->device_id, p_desc[i].dev->vendor_id, p_desc[i].dev);
        if (ret != EOK) {
            continue;
        }
        p_desc[i].probe_fn(p_desc[i].dev);
    }

    return EOK;
}

uint32_t pci_dev_read32(const struct pci_device *device, uint8_t reg)
{
    return pci_config_read32(device->bus_no, device->dev_no, device->function, reg);
}

uint16_t pci_dev_read16(const struct pci_device *device, uint8_t reg)
{
    return pci_config_read16(device->bus_no, device->dev_no, device->function, reg);
}

uint8_t pci_dev_read8(const struct pci_device *device, uint8_t reg)
{
    return pci_config_read8(device->bus_no, device->dev_no, device->function, reg);
}

void pci_dev_write32(const struct pci_device *device, uint8_t reg, uint32_t val)
{
    pci_config_write32(device->bus_no, device->dev_no, device->function, reg, val);
}

void pci_dev_write16(const struct pci_device *device, uint8_t reg, uint16_t val)
{
    pci_config_write16(device->bus_no, device->dev_no, device->function, reg, val);
}

void pci_dev_write8(const struct pci_device *device, uint8_t reg, uint8_t val)
{
    pci_config_write8(device->bus_no, device->dev_no, device->function, reg, val);
}

error_t pci_enable_bus_mastering(const struct pci_device *device, bool enable)
{
    if (NULL == device) {
        return EINVAL;
    }
    uint32_t command_status = pci_dev_read32(device, PCI_REG_STATUS_COMMAND);
    if (enable) {
        command_status |= (1 << 2);
    } else {
        command_status &= ~(1 << 2);
    }
    pci_dev_write32(device, PCI_REG_STATUS_COMMAND, command_status);
    return EOK;
}

static uint8_t pci_get_msix_cap(const struct pci_device *device)
{
    uint8_t msix_cap = 0;
    for (int i = 0; i < PCI_CFG_NUM_CAPABILITIES; i++) {
        if (device->capabilities[i].id == PCI_CAP_ID_MSIX) {
            msix_cap = device->capabilities[i].offset;
            break;
        }
    }
    return msix_cap;
}

error_t pci_enable_msix(const struct pci_device *device, bool enable)
{
    if (NULL == device) {
        return EINVAL;
    }
    uint8_t msix_cap = pci_get_msix_cap(device);
    if (0 == msix_cap) {
        return ENODEV;
    }
    uint16_t message_control = pci_dev_read16(device, msix_cap + 0x02);
    message_control |= (1 << 15);
    if (enable) {
        message_control |= (1 << 15);
    } else {
        message_control &= ~(1 << 15);
    }
    pci_dev_write16(device, msix_cap + 0x02, message_control);
    return EOK;
}

error_t pci_get_msix_table_address(const struct pci_device *device, uint64_t *p_addr)
{
    if (NULL == device || NULL == p_addr) {
        return EINVAL;
    }
    uint8_t msix_cap = pci_get_msix_cap(device);
    if (0 == msix_cap) {
        return ENODEV;
    }

    const uint32_t table_addr = pci_dev_read32(device, msix_cap + 0x08);
    const uint32_t base_addr = device->BAR[table_addr & 0x7];
    *p_addr = base_addr + (table_addr & ~0x7);
    return EOK;
}

error_t pci_get_msix_table_size(const struct pci_device *device, uint32_t *p_size)
{
    if (NULL == device || NULL == p_size) {
        return EINVAL;
    }
    uint8_t msix_cap = pci_get_msix_cap(device);
    if (0 == msix_cap) {
        return ENODEV;
    }

    const uint16_t message_control = pci_dev_read16(device, msix_cap + 0x02);
    *p_size = (message_control & 0x3FF) + 1;
    return EOK;
}
