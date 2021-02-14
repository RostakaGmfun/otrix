#pragma once

#include "dev/virtio.hpp"

namespace otrix::dev
{

class virtio_net final: public virtio_dev
{
public:
    virtio_net(pci_device *pci_dev);
    ~virtio_net() override;

    void print_info() override;

    static bool is_virtio_net_device(pci_device *p_dev)
    {
        if (nullptr == p_dev) {
            return false;
        }
        return p_dev->vendor_id == 0x1af4 && p_dev->device_id == 0x1000;
    }

    void get_mac(uint8_t (&mac)[6]);

protected:
    uint32_t negotiate_features(uint32_t device_features) override;

    enum virtio_net_registers {
        mac_0 = 0x14,
        mac_1 = 0x15,
        mac_2 = 0x16,
        mac_3 = 0x17,
        mac_4 = 0x18,
        mac_5 = 0x19,
        net_status = 0x1a,
        max_virtqueue_pairs = 0x1c,
    };

    uint32_t read_reg(uint16_t reg) override;
    void write_reg(uint16_t reg, uint32_t value) override;

private:
    virtq *tx_q_;
    virtq *rx_q_;
};

} // namespace otrix::dev
