#include "dev/virtio_net.hpp"
#include "otrix/immediate_console.hpp"

namespace otrix::dev
{

using otrix::immediate_console;

static constexpr uint16_t tx_queue_size = 256;
static constexpr uint16_t rx_queue_size = 256;
static constexpr uint16_t control_queue_size = 64;

virtio_net::virtio_net(pci_device *pci_dev): virtio_dev(pci_dev)
{
    error_t ret = virtq_create(0, tx_queue_size, &tx_q_);
    if (EOK != ret) {
        immediate_console::print("Failed to create TX queue of size %d\n", tx_queue_size);
    }

    ret = virtq_create(1, rx_queue_size, &rx_q_);
    if (EOK != ret) {
        immediate_console::print("Failed to create RX queue of size %d\n", tx_queue_size);
    }

    ret = virtq_create(2, control_queue_size, &control_q_);
    if (EOK != ret) {
        immediate_console::print("Failed to create Control queue of size %d\n", tx_queue_size);
    }

    init_finished();
}

uint32_t virtio_net::negotiate_features(uint32_t device_features)
{
    return device_features;
}

} // namespace otrix::dev
