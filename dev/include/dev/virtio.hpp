#pragma once

#include <cstdint>
#include "dev/pci.h"

namespace otrix::dev
{

class virtio_dev
{
public:
    virtio_dev(pci_device *pci_dev);

    ~virtio_dev();

    bool valid() const
    {
        return valid_;
    }

    virtual void print_info();

protected:

    enum virtio_device_register
    {
        device_features    = 0x00,
        driver_features    = 0x04,
        queue_address      = 0x08,
        queue_size         = 0x0c,
        queue_select       = 0x0e,
        queue_notify       = 0x10,
        device_status      = 0x12,
        isr_status         = 0x13,
        config_msix_vector = 0x14,
        queue_msix_vector  = 0x16,
    };

    uint32_t read_reg(virtio_device_register reg);
    void write_reg(virtio_device_register reg, uint32_t value);

private:

    bool valid_;

    struct virtio_pci_cap_t
    {
        uint32_t  base;
        uint32_t offset;
        uint32_t len;
    };

    virtio_pci_cap_t caps_[5];

    pci_device *pci_dev_;
};

} // namespace otrix::dev
