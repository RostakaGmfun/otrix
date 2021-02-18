#pragma once

#include "dev/virtio.hpp"

namespace otrix::dev
{

class virtio_net final: public virtio_dev
{
public:
    virtio_net(pci_dev *p_dev);
    ~virtio_net() override;

    void print_info() override;

    static bool is_virtio_net_device(pci_dev *p_dev)
    {
        if (nullptr == p_dev) {
            return false;
        }
        return p_dev->vendor_id() == 0x1af4 && p_dev->device_id() == 0x1000;
    }

    void get_mac(uint8_t (&mac)[6]);

protected:
    uint32_t negotiate_features(uint32_t device_features) override;

    enum virtio_net_registers {
        mac_0 = 0x18,
        mac_1 = 0x19,
        mac_2 = 0x1a,
        mac_3 = 0x1b,
        mac_4 = 0x1c,
        mac_5 = 0x1d,
        net_status = 0x1e,
        max_virtqueue_pairs = 0x20,
    };

    uint32_t read_reg(uint16_t reg) override;
    void write_reg(uint16_t reg, uint32_t value) override;

private:
    virtq *tx_q_;
    virtq *rx_q_;
};

} // namespace otrix::dev
