#pragma once

#include "dev/virtio.hpp"
#include "net/linkif.hpp"
#include "common/utils.h"
#include "kernel/semaphore.hpp"
#include "kernel/msgq.hpp"
#include "kernel/kthread.hpp"

#include <tuple>

namespace otrix::dev
{

class virtio_net final: public virtio_dev, public net::linkif
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

    void get_mac(net::mac_t *p_out) override;

    kerror_t write(net::sockbuf *data, const net::mac_t &dest, net::ethertype type, uint64_t timeout_ms) override;
    kerror_t subscribe_to_rx(net::ethertype type, net::l3_handler_t p_handler, void *ctx) override;

    size_t headers_size() const override;

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

    static void tx_completion_event(void *ctx, void *data_ctx, void *data, size_t size);
    static void rx_handler(void *ctx, void *data_ctx, void *data, size_t size);
    void rx_thread();

    virtq *tx_q_;
    virtq *rx_q_;

    static constexpr auto MAX_RX_HANDLERS = 2;
    std::tuple<net::ethertype, net::l3_handler_t, void *> rx_handlers_[MAX_RX_HANDLERS];

    msgq rx_packet_queue_;
    kthread rx_thread_;
    net::mac_t addr_;

    static constexpr auto MTU = 1514;
};

} // namespace otrix::dev
