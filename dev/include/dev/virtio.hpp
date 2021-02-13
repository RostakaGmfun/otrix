#pragma once

#include <cstdint>
#include <cstddef>
#include "dev/pci.h"
#include "common/error.h"
#include "common/list.h"

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

    /**
     * Create virtqueue with @c index.
     * This allocates required amount of pages to hold descritor and avail/used rings.
     * @param[in] index Index of queue to use.
     * @param[in,out] p_queue_size (In) Max size of queue to create, (out) actual size of queue created.
     * @retval ENODEV Available queue size is 0.
     * @retval EOK Queue created
     */
    error_t create_virtqueue(uint16_t index, uint16_t *p_queue_size);

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

    struct virtqueue {
        intrusive_list list_node;
        int index;
        uint16_t size;
        void *allocated_mem;
        virtio_descriptor *desc_table;
        virtio_ring_hdr *avail_ring_hdr;
        uint16_t *avail_ring;
        virtio_ring_hdr *used_ring_hdr;
        virtio_used_elem *used_ring;
    };

    intrusive_list *virtqueues_;
};

} // namespace otrix::dev
