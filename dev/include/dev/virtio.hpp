#pragma once

#include <cstdint>
#include <cstddef>
#include "dev/pci.hpp"
#include "common/error.h"

namespace otrix::dev
{

class virtio_dev
{
public:
    virtio_dev(pci_dev *p_dev);

    virtual ~virtio_dev();

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

    virtual uint32_t read_reg(uint16_t reg);
    virtual void write_reg(uint16_t reg, uint32_t value);

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

    typedef void (*vq_irq_handler_t)(void *ctx, void *data_ctx, void *data, size_t len);

    struct virtq {
        int index;
        uint16_t size;
        void *allocated_mem;
        volatile virtio_descriptor *desc_table;
        volatile virtio_ring_hdr *avail_ring_hdr;
        volatile uint16_t *avail_ring;
        volatile virtio_ring_hdr *used_ring_hdr;
        volatile virtio_used_elem *used_ring;
        void **desc_ctx; // array of descriptor contexts specified in virtq_send_buffer
        uint16_t used_idx;
        int num_free_descriptors;
        int free_list;
        vq_irq_handler_t irq_handler;
        void *irq_handler_ctx;
    };

    /**
     * Create virtqueue with @c index.
     * This allocates required amount of pages to hold descritor and avail/used rings.
     * @param[in] index Index of queue to use.
     * @param[out] p_out_virtq Pointer to the created virtqueue handle.
     * @param[in] p_handler If not nullptr, MSI-X is enabled for this queue,
     *                      and the handler will be called when
     * @parampin] p_handler_context Context for the IRQ handler.
     * @retval ENODEV Available queue size is 0.
     * @retval E_OK Queue created
     */
    kerror_t virtq_create(uint16_t index, virtq **p_out_virtq, vq_irq_handler_t = nullptr,
            void *p_handler_context = nullptr);

    kerror_t virtq_destroy(virtq *p_vq);

    /**
     * Add new entry to the descriptor table and put it into the available ring.
     */
    kerror_t virtq_send_buffer(virtq *p_vq, void *p_buffer, uint32_t buffer_size,
            bool device_writable, void *buf_ctx = nullptr);

    virtual uint32_t negotiate_features(uint32_t device_features);

    void begin_init();

    void init_finished();

    pci_dev *pci_dev_;

private:
    static void handle_vq_irq(void *ctx);

private:
    bool valid_;
    uint32_t features_;
};

} // namespace otrix::dev
