#pragma once

#include "dev/virtio.hpp"

namespace otrix::dev
{

class virtio_console final: public virtio_dev
{
public:
    virtio_console(pci_dev *p_dev);
    ~virtio_console() override;

    static bool is_virtio_console_device(pci_dev *p_dev)
    {
        if (nullptr == p_dev) {
            return false;
        }
        return p_dev->vendor_id() == 0x1af4 && p_dev->device_id() == 0x1003;
    }

protected:
    uint32_t negotiate_features(uint32_t device_features) override;

    enum virtio_console_registers {
        console_cols = 0x18,
        console_rows = 0x1a,
        console_max_nr_ports = 0x1c,
        console_emerg_write = 0x20,
    };

    uint32_t read_reg(uint16_t reg) override;
    void write_reg(uint16_t reg, uint32_t value) override;

private:
    virtq *tx_q_;
    virtq *rx_q_;
};

} // namespace otrix::dev
