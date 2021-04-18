#include "dev/virtio_blk.hpp"
#include "otrix/immediate_console.hpp"
#include "arch/asm.h"

namespace otrix::dev
{

enum virtio_blk_features
{
    VIRTIO_BLK_F_SIZE_MAX = 1,
    VIRTIO_BLK_F_SEG_MAX = 2,
    VIRTIO_BLK_F_GEOMETRY = 4,
    VIRTIO_BLK_F_RO = 5,
    VIRTIO_BLK_F_BLK_SIZE = 6,
    VIRTIO_BLK_F_FLUSH = 9,
    VIRTIO_BLK_F_TOPOLOGY = 10,
    VIRTIO_BLK_F_CONFIG_WCR = 11,
};

struct virtio_blk_req_hdr {
    uint32_t type;
    uint32_t reserved;
    uint32_t sector;
};

virtio_blk::virtio_blk(pci_dev *p_dev): virtio_dev(p_dev)
{
    begin_init();

    auto request_completion = [] (void *ctx, void *data_ctx, void *data, size_t size) {
        virtio_blk *p_this = (virtio_blk *)ctx;
        p_this->handle_request_completion(data_ctx, data, size);
    };

    kerror_t ret = virtq_create(0, &request_q_, request_completion, this);
    if (E_OK != ret) {
        immediate_console::print("Failed to create TX queue\n");
    }
    init_finished();
}

virtio_blk::~virtio_blk()
{
    if (nullptr != request_q_) {
        virtq_destroy(request_q_);
    }
}

uint32_t virtio_blk::negotiate_features(uint32_t device_features)
{
    device_features = virtio_dev::negotiate_features(device_features);
    const uint32_t supported_features =
        (1 << VIRTIO_BLK_F_SEG_MAX) |
        (1 << VIRTIO_BLK_F_TOPOLOGY) |
        (1 << VIRTIO_BLK_F_BLK_SIZE) |
        (1 << VIRTIO_BLK_F_GEOMETRY) |
        (1 << VIRTIO_BLK_F_FLUSH);
    if ((supported_features & device_features) != supported_features) {
        immediate_console::print("Failed to negotiate required features: %08x, %08x\n", supported_features, device_features);
    }
    return (device_features & 0xFF000000) | (device_features & supported_features);
}

void virtio_blk::print_info()
{
    virtio_dev::print_info();
    const uint32_t cap_low = read_reg(capacity1);
    const uint32_t cap_high = read_reg(capacity2);
    immediate_console::print("Capacity: %d bytes\n", (cap_low | (uint64_t)cap_high << 32) * 512);
}

uint32_t virtio_blk::read_reg(uint16_t reg)
{
    switch (reg) {
    case capacity1:
    case capacity2:
    case size_max:
    case seg_max:
    case geometry:
    case blk_size:
        return arch_io_read32(pci_dev_->bar(0) + reg);
    default:
        return virtio_dev::read_reg(reg);
    }
}

void virtio_blk::write_reg(uint16_t reg, uint32_t value)
{
    switch (reg) {
    case capacity1:
    case capacity2:
    case size_max:
    case seg_max:
    case geometry:
    case blk_size:
        arch_io_write32(pci_dev_->bar(0) + reg, value);
        break;
    default:
        virtio_dev::write_reg(reg, value);
    }
}


void virtio_blk::handle_request_completion(void *data_ctx, void *data, size_t size)
{
    // TODO
}

} // otrix::dev
