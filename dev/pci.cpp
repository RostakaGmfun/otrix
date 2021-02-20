#include "dev/pci.hpp"

#include <cstring>

#include "arch/asm.h"
#include "otrix/immediate_console.hpp"

#include "arch/lapic.hpp"
#include "arch/irq_manager.hpp"

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

#define PCI_REG_DEVICE_ID(reg) ((reg & 0xFFFF0000) >> 16)
#define PCI_REG_VENDOR_ID(reg) (reg & 0x0000FFFF)
#define PCI_REG_ILLEGAL_VENDOR 0xFFFF

#define PCI_REG_STATUS(reg) ((reg & 0xFFFF0000) >> 16)
#define PCI_REG_COMMAND(reg) (reg & 0x0000FFFF)

#define PCI_REG_CLASS_CODE(reg) ((reg & 0xFF000000) >> 24)
#define PCI_REG_SUBCLASS(reg) ((reg & 0x00FF0000) >> 16)
#define PCI_REG_PROG_IF(reg) ((reg & 0x0000FF00) >> 8)
#define PCI_REG_REVISION_ID(reg) (reg & 0x000000FF)

#define PCI_REG_BIST(reg) ((reg & 0xFF000000) >> 24)
#define PCI_REG_HDR_TYPE(reg) ((reg & 0x00FF0000) >> 16)

#define PCI_REG_SUBSYSTEM_ID(reg) ((reg & 0xFFFF0000) >> 16)
#define PCI_REG_SUBSYSTEM_VENDOR_ID(reg) (reg & 0x0000FFFF)

#define PCI_REG_SUBSYS_ID(ret) ((reg & 0xFFFF0000) >> 16)
#define PCI_REG_SUBSYS_VENDOR_ID(ret) ((reg & 0x0000FFFF))

#define PCI_REG_GET_CAP_PTR(reg) (reg & 0x000000FC)

#define PCI_REG_INT_PIN(reg) ((reg & 0x0000FF00) >> 8)
#define PCI_REG_INT_LINE(reg) (reg & 0x000000FF)

#define PCI_CAP_ID 0x00
#define PCI_CAP_NEXT 0x01

#define PCI_REG_STATUS_CAPLIST (1 << 4)

#define PCI_CAP_ID_NULL   0x00
#define PCI_CAP_ID_MSIX   0x11
#define PCI_CAP_ID_VENDOR 0x09

namespace otrix::dev
{

enum pci_registers
{
    PCI_REG_DEVICE_VENDOR = 0x00,
    PCI_REG_STATUS_COMMAND = 0x04,
    PCI_REG_CLASS_SUBCLASS_REV_ID = 0x08,
    PCI_REG_BIST_HDR_TYPE = 0x0C,
    PCI_REG_SUBSYSTEM_ID = 0x2C,
    PCI_REG_BAR0 = 0x10,
    PCI_REG_BAR1 = 0x14,
    PCI_REG_BAR2 = 0x18,
    PCI_REG_BAR3 = 0x1C,
    PCI_REG_BAR4 = 0x20,
    PCI_REG_BAR5 = 0x24,
    PCI_REG_SUBSYSTEM = 0x2C,
    PCI_REG_CAP_PTR = 0x34,
    PCI_REG_INTERRUPT = 0x3C,
};

using otrix::immediate_console;

/**
 * Read single 32-bit register from PCI Configuration space
 * for specified bus, device and function.
 */
static inline uint32_t pci_config_read32(const uint8_t bus, const uint8_t dev,
        const uint8_t fun, const uint8_t reg) {
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
        const uint8_t fun, uint8_t addr) {
    uint8_t aligned_addr = addr & (~0x03);

    const uint32_t data = pci_config_read32(bus, dev, fun, aligned_addr);
    return (data >> ((addr & 0x01) * 16)) & 0xFFFF;
}

/**
 * Read byte from unaligned address of PCI configuration space.
 */
static inline uint8_t pci_config_read8(const uint8_t bus, const uint8_t dev,
        const uint8_t fun, uint8_t addr) {
    uint8_t aligned_addr = addr & (~0x03);

    const uint32_t data = pci_config_read32(bus, dev, fun, aligned_addr);
    return (data >> ((addr & 0x03) * 8)) & 0xFF;
}

/**
 * Write single 32-bit register from PCI Configuration space
 * for specified bus, device and function.
 */
static inline void pci_config_write32(const uint8_t bus, const uint8_t dev,
        const uint8_t fun, const uint8_t reg, uint32_t val) {
    long flags = arch_irq_save();
    arch_io_write32(PCI_CFG_ADDR, PCI_CFG_REG(bus, dev, fun, reg));
    arch_io_write32(PCI_CFG_DATA, val);
    arch_irq_restore(flags);
}

/**
 * Write 16-bit value to unaligned address of PCI configuration space.
 */
static inline void pci_config_write16(const uint8_t bus, const uint8_t dev,
        const uint8_t fun, uint8_t addr, uint16_t val) {
    uint8_t aligned_addr = addr & (~0x03);

    const uint32_t data = pci_config_read32(bus, dev, fun, aligned_addr);
    pci_config_write32(bus, dev, fun, aligned_addr, (data & ~(0xffff << ((addr & 0x01) * 16))) | val << ((addr & 0x01) * 16));
}

/**
 * Write byte from unaligned address of PCI configuration space.
 */
static inline void pci_config_write8(const uint8_t bus, const uint8_t dev,
        const uint8_t fun, uint8_t addr, uint8_t val) {
    uint8_t aligned_addr = addr & (~0x03);

    const uint32_t data = pci_config_read32(bus, dev, fun, aligned_addr);
    pci_config_write32(bus, dev, fun, aligned_addr, (data & ~(0xff << ((addr & 0x03) * 8))) | val << ((addr & 0x03) * 8));
}

pci_dev::pci_dev(uint8_t bus, uint8_t dev, uint8_t function,
        uint16_t device_id, uint16_t vendor_id):
    bus_(bus), dev_(dev), function_(function),
    device_id_(device_id), vendor_id_(vendor_id),
    msix_table_(nullptr), msix_table_size_(0) {
    for (int i = 0; i < NUM_BARS; i++) {
        bar_[i] = cfg_read32(PCI_REG_BAR0 + i * 4);
        if (bar_[i] & 1) {
            // IO space
            bar_[i] &= 0xFFFC;
        }
    }

    const uint16_t status = PCI_REG_STATUS(cfg_read32(PCI_REG_STATUS_COMMAND));

    if (status & PCI_REG_STATUS_CAPLIST) {
        read_capabilities();
    }
}

pci_dev::~pci_dev() {

}

void pci_dev::read_capabilities() {
    size_t cap = 0;
    uint8_t next = PCI_REG_GET_CAP_PTR(cfg_read32(PCI_REG_CAP_PTR));
    do {
        capabilities_[cap].first = cfg_read8(next);
        capabilities_[cap].second = next;
        next = cfg_read8(next + 1);
        if (0 != capabilities_[cap].first) {
            // Don't save NULL capabilities
            cap++;
        }
    } while (next != 0x00 && cap < MAX_CAPABILITIES);
}

uint8_t pci_dev::get_cap(uint8_t cap_id) const {
    for (int i = 0; i < MAX_CAPABILITIES; i++) {
        if (capabilities_[i].first == cap_id) {
            return capabilities_[i].second;
        }
    }
    return 0;
}


kerror_t pci_dev::find_devices(pci_dev::descriptor_t *p_targets, size_t num_targets)
{
    if (nullptr == p_targets || 0 == num_targets) {
        return E_INVAL;
    }

    for (int bus = 0; bus < PCI_ADDR_MAX_BUSES - 1; bus++) {
        for (int dev = 0; dev < PCI_ADDR_MAX_DEVICES - 1; dev++) {
            uint32_t dev_vend = pci_config_read32(bus, dev, 0, PCI_REG_DEVICE_VENDOR);
            if (PCI_REG_VENDOR_ID(dev_vend) == PCI_REG_ILLEGAL_VENDOR) {
                continue;
            }

            const uint16_t device_id = PCI_REG_DEVICE_ID(dev_vend);
            const uint16_t vendor_id = PCI_REG_VENDOR_ID(dev_vend);

            for (size_t i = 0; i < num_targets; i++) {
                if (device_id == p_targets[i].device_id
                        && vendor_id == p_targets[i].vendor_id) {
                    if (nullptr == p_targets[i].p_dev) {
                        p_targets[i].p_dev = new pci_dev(bus, dev, 0, device_id, vendor_id);
                    }
                    break;
                }
            }
        }
    }

    return E_OK;
}

uint8_t pci_dev::cfg_read8(uint8_t reg) {
    return pci_config_read8(bus_, dev_, function_, reg);
}

uint16_t pci_dev::cfg_read16(uint8_t reg) {
    return pci_config_read16(bus_, dev_, function_, reg);
}

uint32_t pci_dev::cfg_read32(uint8_t reg) {
    return pci_config_read32(bus_, dev_, function_, reg);
}

void pci_dev::cfg_write8(uint8_t reg, uint8_t value) {
    pci_config_write8(bus_, dev_, function_, reg, value);
}

void pci_dev::cfg_write16(uint8_t reg, uint16_t value) {
    pci_config_write16(bus_, dev_, function_, reg, value);
}

void pci_dev::cfg_write32(uint8_t reg, uint32_t value) {
    pci_config_write32(bus_, dev_, function_, reg, value);
}

kerror_t pci_dev::enable_bus_mastering(bool enable) {
    uint32_t command_status = cfg_read32(PCI_REG_STATUS_COMMAND);
    if (enable) {
        command_status |= (1 << 2);
    } else {
        command_status &= ~(1 << 2);
    }
    cfg_write32(PCI_REG_STATUS_COMMAND, command_status);
    return E_OK;
}

kerror_t pci_dev::enable_msix(bool enable) {
    if (nullptr != msix_table_) {
        return E_OK;
    }
    const uint8_t msix_cap = get_cap(PCI_CAP_ID_MSIX);
    if (0 == msix_cap) {
        return E_NODEV;
    }

    uint32_t message_control = cfg_read32(msix_cap);
    if (enable) {
        cfg_write32(msix_cap, message_control | (1 << 31));
    } else {
        cfg_write32(msix_cap, message_control & ~(1 << 31));
        return E_OK;
    }

    const uint32_t command_status = cfg_read32(PCI_REG_STATUS_COMMAND);
    // Disable INTx
    cfg_write32(PCI_REG_STATUS_COMMAND, command_status | (1 << 10));

    const uint32_t table_addr = cfg_read32(msix_cap + 0x04);
    const uint32_t base_addr = bar(table_addr & 0x7) & ~0xf;
    msix_table_ = reinterpret_cast<msix_entry_t *>(base_addr + (table_addr & ~0x7));

    msix_table_size_ = ((message_control >> 16) & 0x3FF) + 1;

    for (int i = 0; i < msix_table_size_; i++) {
        msix_table_[i].message_address_low = 0;
        msix_table_[i].vector_control = 1;
    }
    cfg_write32(msix_cap, cfg_read32(msix_cap) & ~(1 << 30)); // Unmask

    return E_OK;
}

std::pair<kerror_t, uint16_t> pci_dev::request_msix(void (*p_handler)(void *), const char *p_owner, void *p_handler_context) {
    uint16_t msix_vector = 0xFFFF;

    if (nullptr == p_handler) {
        return {E_INVAL, msix_vector};
    }

    for (uint16_t i = 0; i < msix_table_size_; i++) {
        if (0 == msix_table_[i].message_address_low) {
            msix_vector = i;
            break;
        }
    }

    if (0xFFFF == msix_vector) {
        return {E_NOMEM, msix_vector};
    }

    const uint8_t irq = otrix::arch::irq_manager::request_irq(p_handler, p_owner, p_handler_context);

    // RH = 1
    msix_table_[msix_vector].vector_control = 0x1;
    msix_table_[msix_vector].message_address_low = 0xFEE00000 |
        (otrix::arch::local_apic::id() << 12);
    msix_table_[msix_vector].message_address_high = 0;
    // Edge triggered, fixed
    msix_table_[msix_vector].message_data = irq;
    msix_table_[msix_vector].vector_control = 0x0;

    return {E_OK, msix_vector};
}

void pci_dev::free_msix(uint16_t vector) {
    if (vector >= msix_table_size_) {
        return;
    }
    msix_table_[vector].vector_control = 1;
    msix_table_[vector].message_address_low = 0;
    msix_table_[vector].message_data = 0;
}

void pci_dev::print_info() const {
    immediate_console::print("PCI device %02x:%02x: device_id %x, vendor_id %x\n",
            bus_, dev_, device_id_, vendor_id_);
    for (int i = 0; i < NUM_BARS; i++) {
        immediate_console::print("\tBAR%d 0x%08x\n", i, bar_[i]);
    }
    immediate_console::print("\tCapabilities:\n");
    for (int i = 0; i < MAX_CAPABILITIES; i++) {
        if (0 == capabilities_[i].first) {
            break;
        }
        immediate_console::print("\t\tCAP %02x %02x\n", capabilities_[i].first, capabilities_[i].second);
    }
    if (nullptr != msix_table_) {
        immediate_console::print("MSI-X @ %p of size %d\n", msix_table_, msix_table_size_);

        for (uint16_t i = 0; i < msix_table_size_; i++) {
            if (0 != msix_table_[i].message_address_low) {
                immediate_console::print("\tMSI-X/%d: %d\n", i, msix_table_[i].message_data & 0xFF);
            }
        }
    }
}

} // namespace otrix::dev
