#include "dev/virtio_net.hpp"
#include "otrix/immediate_console.hpp"
#include "arch/asm.h"

#define VIRTIO_NET_S_LINK_UP  1
#define VIRTIO_NET_S_ANNOUNCE 2

namespace otrix::dev
{

using otrix::immediate_console;

virtio_net::virtio_net(pci_device *pci_dev): virtio_dev(pci_dev)
{
    begin_init();

    error_t ret = virtq_create(0, &tx_q_);
    if (EOK != ret) {
        immediate_console::print("Failed to create TX queue\n");
    }

    ret = virtq_create(1, &rx_q_);
    if (EOK != ret) {
        immediate_console::print("Failed to create RX queue\n");
    }

    ret = virtq_create(2, &control_q_);
    if (EOK != ret) {
        immediate_console::print("Failed to create Control queue\n");
    }

    init_finished();
}

virtio_net::~virtio_net()
{
    if (nullptr != tx_q_) {
        virtq_destroy(tx_q_);
    }
    if (nullptr != rx_q_) {
        virtq_destroy(rx_q_);
    }
    if (nullptr != control_q_) {
        virtq_destroy(control_q_);
    }
}

void virtio_net::print_info()
{
    virtio_dev::print_info();
    uint8_t mac[6];
    get_mac(mac);
    immediate_console::print("Virtio dev mac: %02x:%02x:%02x:%02x:%02x:%02x, status %04x, max_virtqueue_pairs %d\n",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], read_reg(net_status), read_reg(max_virtqueue_pairs));
}

void virtio_net::get_mac(uint8_t (&mac)[6])
{
    mac[0] = read_reg(mac_0);
    mac[1] = read_reg(mac_1);
    mac[2] = read_reg(mac_2);
    mac[3] = read_reg(mac_3);
    mac[4] = read_reg(mac_4);
    mac[5] = read_reg(mac_5);
}

uint32_t virtio_net::negotiate_features(uint32_t device_features)
{
    return device_features;
}

uint32_t virtio_net::read_reg(uint16_t reg)
{
    switch (reg) {
    case mac_0:
    case mac_1:
    case mac_2:
    case mac_3:
    case mac_4:
    case mac_5:
        return arch_io_read8(pci_dev_->BAR[0] + reg);
    case net_status:
    case max_virtqueue_pairs:
        return arch_io_read16(pci_dev_->BAR[0] + reg);
    default:
        return virtio_dev::read_reg(reg);
    }
}

void virtio_net::write_reg(uint16_t reg, uint32_t value)
{
    switch (reg) {
    case mac_0:
    case mac_1:
    case mac_2:
    case mac_3:
    case mac_4:
    case mac_5:
        arch_io_write8(pci_dev_->BAR[0] + reg, value);
        break;
    case net_status:
    case max_virtqueue_pairs:
        arch_io_write16(pci_dev_->BAR[0] + reg, value);
        break;
    default:
        virtio_dev::write_reg(reg, value);
        break;
    }
}

} // namespace otrix::dev
