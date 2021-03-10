#include "dev/virtio_net.hpp"
#include "otrix/immediate_console.hpp"
#include "arch/asm.h"
#include "kernel/alloc.hpp"
#include "kernel/kthread.hpp"
#include "common/utils.h"
#include "net/ethernet.hpp"
#include "net/sockbuf.hpp"

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

virtio_net::virtio_net(pci_dev *p_dev): virtio_dev(p_dev), rx_packet_queue_(16, sizeof(net::sockbuf *)),
                                        rx_thread_(64 * 1024, [] (void *ctx) { ((virtio_net *)ctx)->rx_thread(); }, "virtio_net-RX", 3, this)
{
    begin_init();

    kerror_t ret = virtq_create(1, &tx_q_, tx_completion_event, this);
    if (E_OK != ret) {
        immediate_console::print("Failed to create TX queue\n");
    }

    ret = virtq_create(0, &rx_q_, rx_handler, this);
    if (E_OK != ret) {
        immediate_console::print("Failed to create RX queue\n");
    }

    init_finished();

    addr_[0] = read_reg(mac_0);
    addr_[1] = read_reg(mac_1);
    addr_[2] = read_reg(mac_2);
    addr_[3] = read_reg(mac_3);
    addr_[4] = read_reg(mac_4);
    addr_[5] = read_reg(mac_5);

    scheduler::get().add_thread(&rx_thread_);
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
    memcpy(mac, addr_, sizeof(addr_));
}

kerror_t virtio_net::write(net::sockbuf *data, const net::mac_t &dest, net::ethertype type, uint64_t timeout_ms)
{
    using namespace net;
    ethernet_hdr *e_hdr = reinterpret_cast<ethernet_hdr *>(data->add_header(sizeof(ethernet_hdr), sockbuf_header_t::ethernet));
    memcpy(&e_hdr->dmac, &dest, sizeof(dest));
    get_mac(&e_hdr->smac);
    if (net::ethertype::unknown == type) {
        e_hdr->ethertype = htons(data->size() - sizeof(ethernet_hdr));
    } else {
        e_hdr->ethertype = htons(static_cast<uint16_t>(type));
    }
    virtio_net_hdr *v_hdr = reinterpret_cast<virtio_net_hdr *>(data->add_header(sizeof(virtio_net_hdr), sockbuf_header_t::virtio));
    memset(v_hdr, 0, sizeof(*v_hdr));

    kerror_t ret = virtq_send_buffer(tx_q_, data->data(), data->size(), false);
    if (E_OK != ret) {
        return ret;
    }
    return tx_complete_.take(timeout_ms) ? E_OK : E_TOUT;
}

kerror_t virtio_net::subscribe_to_rx(net::ethertype type, net::l2_handler_t p_handler, void *ctx)
{
    auto flags = arch_irq_save();
    for (int i = 0; i < MAX_RX_HANDLERS; i++) {
        if (net::ethertype::unknown == std::get<0>(rx_handlers_[i])) {
            std::get<0>(rx_handlers_[i]) = type;
            std::get<1>(rx_handlers_[i]) = p_handler;
            std::get<2>(rx_handlers_[i]) = ctx;
            arch_irq_restore(flags);
            return E_OK;
        }
    }
    arch_irq_restore(flags);
    return E_NOMEM;
}

size_t virtio_net::headers_size() const
{
    return sizeof(virtio_net_hdr) + sizeof(net::ethernet_hdr);
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

void virtio_net::tx_completion_event(void *ctx, void *data, size_t size)
{
    (void)data;
    (void)size;
    virtio_net *p_this = (virtio_net *)ctx;
    p_this->tx_complete_.give();
}

void virtio_net::rx_handler(void *ctx, void *data, size_t size)
{
    virtio_net *p_this = (virtio_net *)ctx;
    if (p_this->rx_packet_queue_.full()) {
        return;
    }
    const auto skb_free_func = [] (void *buf, size_t size, void *ctx) {
        // Return buffer to the rx queue
        virtio_net *p_this = (virtio_net *)ctx;
        p_this->virtq_send_buffer(p_this->rx_q_, buf, size, true);
    };
    // Create zero-copy socket buffer
    // TODO: get rid of memory allocation in IRQ handler
    net::sockbuf *skb = new net::sockbuf((uint8_t *)data, size, skb_free_func, p_this);
    if (nullptr == skb) {
        return;
    }
    if (!p_this->rx_packet_queue_.write(&skb)) {
        // Most likely impossible, because interrupt nesting is disabled
        delete skb;
    }
}

void virtio_net::rx_thread()
{
    using namespace net;
    immediate_console::print("virtio-net rx thread started\n");

    for (int i = 0; i < 16; i++) {
        void *buf = otrix::alloc(MTU + sizeof(virtio_net_hdr));
        // TODO: destroy thread and free buffers in virtio_net desctructor
        virtq_send_buffer(rx_q_, buf, MTU + sizeof(virtio_net_hdr), true);
    }
    while (1) {
        sockbuf *skb = nullptr;
        rx_packet_queue_.read(&skb, -1);
        skb->add_parsed_header(sizeof(virtio_net_hdr), sockbuf_header_t::virtio);
        skb->add_parsed_header(sizeof(ethernet_hdr), sockbuf_header_t::ethernet);
        const ethernet_hdr *e_hdr = (const ethernet_hdr *)skb->header(sockbuf_header_t::ethernet);
        const mac_t broadcast_mac = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
        if (0 != memcmp(e_hdr->dmac, broadcast_mac, sizeof(e_hdr->dmac)) &&
            0 != memcmp(e_hdr->dmac, addr_, sizeof(e_hdr->dmac)))
        {
            delete skb;
            continue;
        }

        for (const auto handler : rx_handlers_) {
            if (std::get<0>(handler) == (ethertype)htons(e_hdr->ethertype)) {
                std::get<1>(handler)(skb, std::get<2>(handler));
            }
        }

        delete skb;
    }
}

} // namespace otrix::dev
