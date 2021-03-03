#include "dev/virtio_net.hpp"
#include "otrix/immediate_console.hpp"
#include "arch/asm.h"
#include "kernel/alloc.hpp"
#include "net/ethernet.hpp"
#include "common/utils.h"

#define VIRTIO_NET_S_LINK_UP  1
#define VIRTIO_NET_S_ANNOUNCE 2

namespace otrix::dev
{

enum virtio_net_features
{
    VIRTIO_NET_F_CSUM = 0,
    VIRTIO_NET_F_GUEST_CSUM = 1,
    VIRTIO_NET_F_CTRL_GUEST_OFFLOADS = 2,
    VIRTIO_NET_F_MAC = 5,
    VIRTIO_NET_F_GUEST_TSO4 = 7,
    VIRTIO_NET_F_GUEST_TSO6 = 8,
    VIRTIO_NET_F_GUEST_ECN = 9,
    VIRTIO_NET_F_GUEST_UFO = 10,
    VIRTIO_NET_F_HOST_TSO4 = 11,
    VIRTIO_NET_F_HOST_TSO6 = 12,
    VIRTIO_NET_F_HOST_ECN = 13,
    VIRTIO_NET_F_HOST_UFO = 14,
    VIRTIO_NET_F_MRG_RXBUF = 15,
    VIRTIO_NET_F_STATUS = 16,
    VIRTIO_NET_F_CTRL_VQ = 17,
    VIRTIO_NET_F_CTRL_RX = 18,
    VIRTIO_NET_F_CTRL_VLAN = 19,
    VIRTIO_NET_F_GUEST_ANNOUNCE = 21,
    VIRTIO_NET_F_MQ = 22,
    VIRTIO_NET_F_CTRL_MAC_ADDR = 23,
};

struct virtio_net_hdr
{
    uint8_t flags;
    uint8_t gso_type;
    uint16_t hdr_len;
    uint16_t gso_size;
    uint16_t csum_start;
    uint16_t csum_offset;
} __attribute__((packed));

using otrix::immediate_console;

virtio_net::virtio_net(pci_dev *p_dev): virtio_dev(p_dev)
{
    begin_init();

    kerror_t ret = virtq_create(1, &tx_q_, tx_completion_event, this);
    if (E_OK != ret) {
        immediate_console::print("Failed to create TX queue\n");
    }

    ret = virtq_create(0, &rx_q_);
    if (E_OK != ret) {
        immediate_console::print("Failed to create RX queue\n");
    }

    init_finished();
    tx_buffer_ = (uint8_t *)otrix::alloc(MTU + sizeof(virtio_net_hdr) + sizeof(net::ethernet_hdr));
    if (nullptr == tx_buffer_) {
        immediate_console::print("Failed to allocate TX buffer\n");
    }
}

virtio_net::~virtio_net()
{
    if (nullptr != tx_q_) {
        virtq_destroy(tx_q_);
    }
    if (nullptr != rx_q_) {
        virtq_destroy(rx_q_);
    }
}

void virtio_net::print_info()
{
    virtio_dev::print_info();
    uint8_t mac[6];
    get_mac(&mac);
    immediate_console::print("Virtio dev mac: %02x:%02x:%02x:%02x:%02x:%02x, status %04x\n",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], read_reg(net_status));
}

void virtio_net::get_mac(net::mac_t *mac)
{
    (*mac)[0] = read_reg(mac_0);
    (*mac)[1] = read_reg(mac_1);
    (*mac)[2] = read_reg(mac_2);
    (*mac)[3] = read_reg(mac_3);
    (*mac)[4] = read_reg(mac_4);
    (*mac)[5] = read_reg(mac_5);
}

kerror_t virtio_net::write(const void *data, size_t size, const net::mac_t &dest, net::ethertype type, uint64_t timeout_ms)
{
    if (size > MTU) {
        return E_NOMEM;
    }

    virtio_net_hdr *hdr = reinterpret_cast<virtio_net_hdr *>(tx_buffer_);
    memset(hdr, 0, sizeof(*hdr));

    net::ethernet_hdr *e_hdr = reinterpret_cast<net::ethernet_hdr *>(tx_buffer_ + sizeof(*hdr));
    memcpy(&e_hdr->dmac, &dest, sizeof(dest));
    get_mac(&e_hdr->smac);
    if (net::ethertype::unknown == type) {
        e_hdr->ethertype = htons(size);
    } else {
        e_hdr->ethertype = htons(static_cast<uint16_t>(type));
    }
    memcpy(tx_buffer_ + sizeof(*hdr) + sizeof(*e_hdr), data, size);
    kerror_t ret = virtq_send_buffer(tx_q_, tx_buffer_, sizeof(*hdr) + sizeof(*e_hdr) + size, false);
    if (E_OK != ret) {
        return ret;
    }
    return tx_complete_.take(timeout_ms) ? E_OK : E_TOUT;
}

size_t virtio_net::read(void *data, size_t size, net::mac_t *src, net::ethertype *type, uint64_t timeout_ms)
{

    (void)data;
    (void)size;
    (void)src;
    (void)type;
    (void)timeout_ms;
    return 0;
}

uint32_t virtio_net::negotiate_features(uint32_t device_features)
{
    const uint32_t supported_features = (1 << VIRTIO_NET_F_MAC) | (1 << VIRTIO_NET_F_STATUS);
    if ((supported_features & device_features) != supported_features) {
        immediate_console::print("Failed to negotiate required features: %08x, %08x\n", supported_features, device_features);
    }
    return (device_features & 0xFF000000) | (device_features & supported_features);
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
        return arch_io_read8(pci_dev_->bar(0) + reg);
    case net_status:
    case max_virtqueue_pairs:
        return arch_io_read16(pci_dev_->bar(0) + reg);
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
        arch_io_write8(pci_dev_->bar(0) + reg, value);
        break;
    case net_status:
    case max_virtqueue_pairs:
        arch_io_write16(pci_dev_->bar(0) + reg, value);
        break;
    default:
        virtio_dev::write_reg(reg, value);
        break;
    }
}

void virtio_net::tx_completion_event(void *ctx)
{
    virtio_net *p_this = (virtio_net *)ctx;
    p_this->tx_complete_.give();
}


} // namespace otrix::dev
