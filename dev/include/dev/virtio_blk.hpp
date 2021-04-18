#pragma once

#include "dev/virtio.hpp"

namespace otrix::dev
{

class virtio_blk: public virtio_dev
{
public:
    virtio_blk(pci_dev *p_dev);
    ~virtio_blk() override;

    void print_info() override;

    static bool is_virtio_blk_device(pci_dev *p_dev)
    {
        if (nullptr == p_dev) {
            return false;
        }
        return p_dev->vendor_id() == 0x1af4 && p_dev->device_id() == 0x1001;
    }

protected:

    uint32_t negotiate_features(uint32_t device_features) override;

    enum virtio_blk_registers {
        capacity1 = 0x18,
        capacity2 = 0x1c,
        size_max  = 0x20,
        seg_max   = 0x24,
        geometry  = 0x28,
        blk_size  = 0x2c,
        // More fields follow, currently unused
    };

    uint32_t read_reg(uint16_t reg) override;
    void write_reg(uint16_t reg, uint32_t value) override;

    void handle_request_completion(void *data_ctx, void *data, size_t size);

    virtq *request_q_;
};

} // namespace otrix::dev
