#pragma once

#include <cstdint>
#include <cstddef>
#include "dev/pci.h"
#include "common/error.h"

namespace otrix::dev
{

class virtio_dev
{
public:
    virtio_dev(pci_device *pci_dev);

    ~virtio_dev();

    bool valid() const
    {
        return valid_;
    }

    virtual void print_info();

protected:

    enum virtio_device_register
    {
        device_features    = 0x00,
        driver_features    = 0x04,
        queue_address      = 0x08,
        queue_size         = 0x0c,
        queue_select       = 0x0e,
        queue_notify       = 0x10,
        device_status      = 0x12,
        isr_status         = 0x13,
        config_msix_vector = 0x14,
        queue_msix_vector  = 0x16,
    };

    uint32_t read_reg(virtio_device_register reg);
    void write_reg(virtio_device_register reg, uint32_t value);

    struct virtio_descriptor
    {
        uint64_t addr;
        uint32_t len;
        uint16_t flags;
        uint16_t next;
    } __attribute__((packed));

    struct virtio_ring_hdr
    {
        uint16_t flags;
        uint16_t idx;
    } __attribute__((packed));

    struct virtio_used_elem
    {
        uint32_t id;
        uint32_t len;
    } __attribute__((packed));

    struct virtq {
        int index;
        uint16_t size;
        void *allocated_mem;
        virtio_descriptor *desc_table;
        virtio_ring_hdr *avail_ring_hdr;
        uint16_t *avail_ring;
        virtio_ring_hdr *used_ring_hdr;
        virtio_used_elem *used_ring;
        int num_used_descriptors;
    };

    /**
     * Create virtqueue with @c index.
     * This allocates required amount of pages to hold descritor and avail/used rings.
     * @param[in] index Index of queue to use.
     * @param[in] p_queue_size Max size of queue to create.
     * @param[out] p_out_virtq Pointer to the created virtqueue handle.
     * @retval ENODEV Available queue size is 0.
     * @retval EOK Queue created
     */
    error_t virtq_create(uint16_t index, uint16_t size, virtq **p_out_virtq);

    error_t virtq_destroy(virtq *p_vq);

    /**
     * Add new entry to the descriptor table and put it into the available ring.
     */
    error_t virtq_add_buffer(virtq *p_vq, uint64_t *p_buffer, uint32_t buffer_size, bool device_writable);

private:

    bool valid_;

    struct virtio_pci_cap_t
    {
        uint32_t  base;
        uint32_t offset;
        uint32_t len;
    };

    virtio_pci_cap_t caps_[5];

    pci_device *pci_dev_;
};

} // namespace otrix::dev
