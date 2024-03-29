#include "dev/virtio.hpp"

#include <algorithm>
#include <cstring>

#include "arch/irq_manager.hpp"
#include "arch/asm.h"
#include "otrix/immediate_console.hpp"
#include "kernel/alloc.hpp"

#define VIRTIO_PCI_VENDOR_ID 0x1af4

#define VIRTIO_F_EVENT_IDX 29

#define VIRTQ_DESC_F_WRITE 2
#define VIRTQ_USED_F_NO_NOTIFY 1

namespace otrix::dev
{

enum virtio_device_status
{
    acknowledge = 1,
    driver = 2,
    failed = 128,
    features_ok = 8,
    driver_ok = 4,
    needs_reset = 64,
};

using otrix::immediate_console;

virtio_dev::virtio_dev(pci_dev *p_dev): pci_dev_(p_dev)
{}

void virtio_dev::begin_init()
{
    if (pci_dev_->vendor_id() != VIRTIO_PCI_VENDOR_ID) {
        return;
    }
    pci_dev_->enable_bus_mastering(true);

    // MSI-X should be enabled,
    // because we expect to have MSI-X registers in Virtio configuration struct
    if (E_OK != pci_dev_->enable_msix(true)) {
        return;
    }

    valid_ = true;

    write_reg(device_status, 0);
    write_reg(device_status, virtio_device_status::acknowledge | virtio_device_status::driver);

    features_ = negotiate_features(read_reg(device_features));
    write_reg(driver_features, features_);
}

virtio_dev::~virtio_dev()
{
}

void virtio_dev::print_info()
{
    immediate_console::print("Device status %02x, features %08x\n", read_reg(device_status), features_);
    int queue_idx = 0;
    uint16_t q_size = 0;
    immediate_console::print("VQs: ");
    do {
        write_reg(queue_select, queue_idx);
        q_size = read_reg(queue_size);
        if (0 == q_size) {
            break;
        }
        immediate_console::print("VQ%d(%d) ", queue_idx, q_size);
        queue_idx++;
        if (queue_idx % 8 == 0) {
            immediate_console::print("\n");
        }
    } while (queue_idx != 0);
    immediate_console::print("\n");
}

uint32_t virtio_dev::read_reg(uint16_t reg)
{
    switch (reg) {
    case device_features:
    case driver_features:
    case queue_address:
        return arch_io_read32(pci_dev_->bar(0) + reg);
    case queue_size:
    case queue_select:
    case queue_notify:
        return arch_io_read16(pci_dev_->bar(0) + reg);
    case device_status:
    case isr_status:
        return arch_io_read8(pci_dev_->bar(0) + reg);
    case config_msix_vector:
    case queue_msix_vector:
        return arch_io_read16(pci_dev_->bar(0) + reg);
    default:
        return 0;
    }
}

void virtio_dev::write_reg(uint16_t reg, uint32_t value)
{
    switch (reg) {
    case device_features:
    case driver_features:
    case queue_address:
        arch_io_write32(pci_dev_->bar(0) + reg, value);
        break;
    case queue_size:
    case queue_select:
    case queue_notify:
        arch_io_write16(pci_dev_->bar(0) + reg, value);
        break;
    case device_status:
    case isr_status:
        arch_io_write8(pci_dev_->bar(0) + reg, value);
        break;
    case config_msix_vector:
    case queue_msix_vector:
        arch_io_write16(pci_dev_->bar(0) + reg, value);
        break;
    }
}

kerror_t virtio_dev::virtq_create(uint16_t index, virtq **p_out_virtq, vq_irq_handler_t p_handler, void *p_handler_context)
{
    write_reg(queue_select, index);
    const uint16_t queue_len = read_reg(queue_size);

    const size_t descriptor_table_size = sizeof(virtio_descriptor) * queue_len;
    const size_t available_ring_size = sizeof(virtio_ring_hdr) + sizeof(uint16_t) * queue_len;
    const size_t used_ring_size = sizeof(virtio_ring_hdr) + sizeof(virtio_used_elem) * queue_len;

    auto vq_align = [] (auto in) {
        return (in + 4095) & ~4095;
    };

    const size_t virtq_size = vq_align(descriptor_table_size + available_ring_size) + vq_align(used_ring_size) +
                              sizeof(void *) * queue_len;

    virtq *p_vq = (virtq *)otrix::alloc(sizeof(virtq));
    if (nullptr == p_vq) {
        return E_NOMEM;
    }
    memset(p_vq, 0, sizeof(virtq));

    p_vq->index = index;
    p_vq->size = queue_len;
    p_vq->irq_handler = p_handler;
    p_vq->irq_handler_ctx = p_handler_context;
    // 4095 to adjust the descriptor pointer to be 4kb aligned
    // TODO: Instead of relying on root heap allocator implement page allocator
    const size_t alloc_size = virtq_size + 4095;
    p_vq->allocated_mem = otrix::alloc(alloc_size);
    if (nullptr == p_vq->allocated_mem) {
        otrix::free(p_vq);
        return E_NOMEM;
    }
    memset(p_vq->allocated_mem, 0, alloc_size);

    p_vq->desc_table =
        reinterpret_cast<virtio_descriptor *>(vq_align((uintptr_t)p_vq->allocated_mem));
    p_vq->avail_ring_hdr = (virtio_ring_hdr *)((uint8_t *)p_vq->desc_table + descriptor_table_size);
    p_vq->avail_ring = (uint16_t *)((uint8_t *)p_vq->avail_ring_hdr + sizeof(virtio_ring_hdr));
    p_vq->used_ring_hdr = (virtio_ring_hdr *)((uint8_t *)p_vq->desc_table +
                vq_align(descriptor_table_size + available_ring_size));
    p_vq->used_ring =
        (virtio_used_elem *)((uint8_t *)p_vq->used_ring_hdr + sizeof(virtio_ring_hdr));
    p_vq->desc_ctx = (void **)((uint8_t *)p_vq->used_ring_hdr + vq_align(used_ring_size));

    for (int i = 0; i < p_vq->size - 1; i++) {
        p_vq->desc_table[i].next = i + 1;
    }
    p_vq->desc_table[p_vq->size - 1].next = -1;
    p_vq->num_free_descriptors = p_vq->size;

    write_reg(queue_address, (uintptr_t)p_vq->desc_table >> 12);

    *p_out_virtq = p_vq;

    auto [err, msix_vector] = pci_dev_->request_msix(handle_vq_irq, "VQ", p_vq);
    if (E_OK == err) {
        write_reg(queue_msix_vector, msix_vector);
    }

    immediate_console::print("Created VQ%d @ %p, msix %04x\n", p_vq->index, p_vq->desc_table,
            read_reg(queue_msix_vector));

    return E_OK;
}

kerror_t virtio_dev::virtq_destroy(virtq *p_vq)
{
    if (nullptr == p_vq) {
        return E_INVAL;
    }

    write_reg(queue_select, p_vq->index);
    write_reg(queue_address, 0);

    otrix::free(p_vq->allocated_mem);
    otrix::free(p_vq);

    return E_OK;
}

kerror_t virtio_dev::virtq_send_buffer(virtq *p_vq, void *p_buffer, uint32_t buffer_size,
        bool device_writable, void *buf_ctx)
{
    if (nullptr == p_vq || nullptr == p_buffer || 0 == buffer_size) {
        return E_INVAL;
    }

    if (p_vq->free_list == -1 || 0 == p_vq->num_free_descriptors) {
        return E_NOMEM;
    }

    auto flags = arch_irq_save();
    const int idx = p_vq->free_list;
    p_vq->free_list = p_vq->desc_table[idx].next;
    arch_irq_restore(flags);

    p_vq->desc_ctx[idx] = buf_ctx;
    p_vq->num_free_descriptors--;
    volatile virtio_descriptor *p_desc = &p_vq->desc_table[idx];
    p_desc->addr = (uint64_t)p_buffer;
    p_desc->len = buffer_size;
    p_desc->flags = device_writable ? VIRTQ_DESC_F_WRITE : 0;
    p_desc->next = 0;

    flags = arch_irq_save();
    p_vq->avail_ring[p_vq->avail_ring_hdr->idx % p_vq->size] = idx;
    const int avail_index = p_vq->avail_ring_hdr->idx + 1;

    // Memory barrier to make sure all changes are visible to the device when index is updated
    __sync_synchronize();
    p_vq->avail_ring_hdr->idx = avail_index;
    arch_irq_restore(flags);

    if (!(p_vq->used_ring_hdr->flags & VIRTQ_USED_F_NO_NOTIFY)) {
        write_reg(queue_notify, p_vq->index);
    }

    return E_OK;
}

uint32_t virtio_dev::negotiate_features(uint32_t device_features)
{
    return device_features & ~(1 << VIRTIO_F_EVENT_IDX);
}

void virtio_dev::init_finished()
{
    write_reg(device_status, read_reg(device_status) | virtio_device_status::driver_ok | virtio_device_status::features_ok);
}

void virtio_dev::handle_vq_irq(void *ctx)
{
    virtq *p_vq = reinterpret_cast<virtq *>(ctx);
    while (p_vq->used_idx != p_vq->used_ring_hdr->idx) {
        const int desc_id = p_vq->used_ring[p_vq->used_idx % p_vq->size].id;
        volatile virtio_descriptor *p_desc = &p_vq->desc_table[desc_id];
        if (nullptr != p_vq->irq_handler) {
            p_vq->irq_handler(p_vq->irq_handler_ctx, p_vq->desc_ctx[desc_id],
                    reinterpret_cast<void *>(p_desc->addr), p_desc->len);
        }
        p_desc->next = p_vq->free_list;
        p_vq->free_list = desc_id;
        p_vq->used_idx++;
        p_vq->num_free_descriptors++;
    }
}

} // namespace otrix::dev
