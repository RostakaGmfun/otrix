#include "dev/virtio_console.hpp"

#include <cstring>
#include "otrix/immediate_console.hpp"
#include "arch/asm.h"
#include "kernel/alloc.hpp"

namespace otrix::dev
{

enum virtio_console_features
{
    VIRTIO_CONSOLE_F_SIZE = 0,
    VIRTIO_CONSOLE_F_MULTIPORT = 1,
    VIRTIO_CONSOLE_F_EMERG_WRITE = 2,
};

using otrix::immediate_console;

virtio_console::virtio_console(pci_dev *p_dev): virtio_dev(p_dev)
{
    begin_init();

    error_t ret = virtq_create(1, &tx_q_, &tx_irq_handler, this);
    if (EOK != ret) {
        immediate_console::print("Failed to create TX queue\n");
    }

    ret = virtq_create(0, &rx_q_);
    if (EOK != ret) {
        immediate_console::print("Failed to create RX queue\n");
    }

    init_finished();
}

virtio_console::~virtio_console()
{
    if (nullptr != tx_q_) {
        virtq_destroy(tx_q_);
    }
    if (nullptr != rx_q_) {
        virtq_destroy(rx_q_);
    }
}

size_t virtio_console::write(const char *data, size_t size)
{
    virtq_send_buffer(tx_q_, (char *)data, size, false);
    return size;
}

uint32_t virtio_console::negotiate_features(uint32_t device_features)
{
    const uint32_t supported_features = (1 << VIRTIO_CONSOLE_F_MULTIPORT) | (1 << VIRTIO_CONSOLE_F_EMERG_WRITE);
    if ((supported_features & device_features) != supported_features) {
        immediate_console::print("Failed to negotiate required features: %08x, %08x\n", supported_features, device_features);
    }
    return (device_features & 0xFF000000) | (device_features & supported_features);
}

uint32_t virtio_console::read_reg(uint16_t reg)
{
    switch (reg) {
    case console_cols:
    case console_rows:
        return arch_io_read16(pci_dev_->bar(0) + reg);
    case console_max_nr_ports:
    case console_emerg_write:
        return arch_io_read32(pci_dev_->bar(0) + reg);
    default:
        return virtio_dev::read_reg(reg);
    }
}

void virtio_console::write_reg(uint16_t reg, uint32_t value)
{
    switch (reg) {
    case console_cols:
    case console_rows:
        return arch_io_write16(pci_dev_->bar(0) + reg, value);
    case console_max_nr_ports:
    case console_emerg_write:
        return arch_io_write32(pci_dev_->bar(0) + reg, value);
    default:
        return virtio_dev::write_reg(reg, value);
    }
}

void virtio_console::tx_irq_handler(void *p_ctx)
{
    virtio_console *p_this = (virtio_console *)p_ctx;
    (void)p_this;
}

} // namespace otrix::dev
