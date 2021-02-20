#pragma once

#include <cstdint>
#include <cstddef>
#include <utility>

#include "common/error.h"

namespace otrix::dev
{

class pci_dev
{
public:
    pci_dev(const pci_dev &other) = delete;
    pci_dev &operator=(const pci_dev &other) = delete;

    ~pci_dev();

    uint16_t device_id() const {
        return device_id_;
    }

    uint16_t vendor_id() const {
        return vendor_id_;
    }

    uint32_t bar(unsigned int num) const {
        if (num < 6) {
            return bar_[num];
        }
        return 0;
    }

    kerror_t enable_bus_mastering(bool enable);

    kerror_t enable_msix(bool enable);

    std::pair<kerror_t, uint16_t> request_msix(void (*p_handler)(void *), const char *p_owner, void *p_handler_context = nullptr);

    void free_msix(uint16_t vector);

    void print_info() const;

    struct descriptor_t
    {
        uint16_t device_id;
        uint16_t vendor_id;
        pci_dev *p_dev;
    };

    /**
     * Scan PCI buses and initialize requested PCI devices.
     */
    static kerror_t find_devices(descriptor_t *p_targets, size_t num_targets);

private:
    pci_dev(uint8_t bus, uint8_t dev, uint8_t function, uint16_t device_id, uint16_t vendor_id);

    uint8_t cfg_read8(uint8_t reg);
    uint16_t cfg_read16(uint8_t reg);
    uint32_t cfg_read32(uint8_t reg);

    void cfg_write8(uint8_t reg, uint8_t value);
    void cfg_write16(uint8_t reg, uint16_t value);
    void cfg_write32(uint8_t reg, uint32_t value);

    void read_capabilities();

    uint8_t get_cap(uint8_t cap_id) const;

private:
    uint8_t bus_;
    uint8_t dev_;
    uint8_t function_;
    uint16_t device_id_;
    uint16_t vendor_id_;

    static constexpr auto MAX_CAPABILITIES = 16;
    std::pair<uint8_t, uint8_t> capabilities_[MAX_CAPABILITIES];

    static constexpr auto NUM_BARS = 6;
    uint32_t bar_[NUM_BARS];

    struct msix_entry_t {
        uint32_t message_address_low;
        uint32_t message_address_high;
        uint32_t message_data;
        uint32_t vector_control;
    } __attribute__((packed));

    volatile msix_entry_t *msix_table_;
    uint16_t msix_table_size_;
};

} // namespace otrix::dev
