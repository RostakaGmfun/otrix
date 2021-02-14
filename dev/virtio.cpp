#include "dev/virtio.hpp"

#include <algorithm>
#include <cstring>

#include "arch/irq_manager.hpp"
#include "arch/asm.h"
#include "otrix/immediate_console.hpp"
#include "kernel/alloc.hpp"


/* Common configuration */
#define VIRTIO_PCI_CAP_COMMON_CFG 1
/* Notifications */
#define VIRTIO_PCI_CAP_NOTIFY_CFG 2
/* ISR Status */
#define VIRTIO_PCI_CAP_ISR_CFG    3
/* Device specific configuration */
#define VIRTIO_PCI_CAP_DEVICE_CFG 4
/* PCI configuration access */
#define VIRTIO_PCI_CAP_PCI_CFG    5

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

virtio_dev::virtio_dev(pci_device *pci_dev): pci_dev_(pci_dev)
{
    if (pci_dev->vendor_id != VIRTIO_PCI_VENDOR_ID) {
        return;
    }
    for (int i = 0; i < PCI_CFG_NUM_CAPABILITIES; i++) {
        if (PCI_CAP_ID_VENDOR == pci_dev->capabilities[i].id) {
            const uint8_t cfg_type = pci_dev_read8(pci_dev, pci_dev->capabilities[i].offset + 3);
            if (cfg_type < 1 || cfg_type > 5) {
                immediate_console::print("Unknown Virtio cfg_type=%d\n", cfg_type);
            continue;
        }

        const uint8_t bar = pci_dev_read8(pci_dev, pci_dev->capabilities[i].offset + 4);
        if (bar > 5) {
            immediate_console::print("Wrong BAR id: %d\n", bar);
            continue;
            }
            caps_[cfg_type - 1].base = pci_dev->BAR[bar];
            caps_[cfg_type - 1].offset = pci_dev_read32(pci_dev, pci_dev->capabilities[i].offset + 8);
            caps_[cfg_type - 1].len = pci_dev_read32(pci_dev, pci_dev->capabilities[i].offset + 12);
        }
    }
    valid_ = true;

    write_reg(device_status, 0);
    write_reg(device_status, virtio_device_status::acknowledge | virtio_device_status::driver);
    write_reg(driver_features, read_reg(device_features) & ~(1 << VIRTIO_F_EVENT_IDX));
    immediate_console::print("Device status %02x, features %08x\n", read_reg(device_status), read_reg(device_features));
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

virtio_dev::~virtio_dev()
{
}

void virtio_dev::print_info()
{
    immediate_console::print("Virtio capabilities:\n");
    for (int i = 0; i < 5; i++) {
        immediate_console::print("CAP%d @ %p, len %d\n", i + 1, caps_[i].base + caps_[i].offset, caps_[i].len);
    }
}

uint32_t virtio_dev::read_reg(virtio_device_register reg)
{
    switch (reg) {
    case device_features:
    case driver_features:
    case queue_address:
        return arch_io_read32(pci_dev_->BAR[0] + reg);
    case queue_size:
    case queue_select:
    case queue_notify:
        return arch_io_read16(pci_dev_->BAR[0] + reg);
    case device_status:
    case isr_status:
        return arch_io_read8(pci_dev_->BAR[0] + reg);
    case config_msix_vector:
    case queue_msix_vector:
        return arch_io_read16(pci_dev_->BAR[0] + reg);
    default:
        return 0;
    }
}

void virtio_dev::write_reg(virtio_device_register reg, uint32_t value)
{
    switch (reg) {
    case device_features:
    case driver_features:
    case queue_address:
        return arch_io_write32(pci_dev_->BAR[0] + reg, value);
    case queue_size:
    case queue_select:
    case queue_notify:
        return arch_io_write16(pci_dev_->BAR[0] + reg, value);
    case device_status:
    case isr_status:
        return arch_io_write8(pci_dev_->BAR[0] + reg, value);
    case config_msix_vector:
    case queue_msix_vector:
        return arch_io_write16(pci_dev_->BAR[0] + reg, value);
    }
}

error_t virtio_dev::virtq_create(uint16_t index, uint16_t size, virtq **p_out_virtq)
{
    write_reg(queue_select, index);
    const uint16_t max_queue_size = read_reg(queue_size);
    if (0 == max_queue_size) {
        return ENODEV;
    }

    const uint16_t queue_len = std::min(size, max_queue_size);
    write_reg(queue_size, queue_len);

    const size_t descriptor_table_size = sizeof(virtio_descriptor) * queue_len;
    const size_t available_ring_size = sizeof(virtio_ring_hdr) + sizeof(uint16_t) * queue_len;
    const size_t used_ring_size = sizeof(virtio_ring_hdr) + sizeof(virtio_used_elem) * queue_len;

    auto vq_align = [] (auto in) {
        return (in + 4095) & ~4095;
    };

    const size_t virtq_size = vq_align(descriptor_table_size + available_ring_size) + vq_align(used_ring_size);
    (void)virtq_size;

    virtq *p_vq = (virtq *)otrix::alloc(sizeof(virtq));
    if (nullptr == p_vq) {
        return ENOMEM;
    }
    memset(p_vq, 0, sizeof(virtq));

    p_vq->index = index;
    p_vq->size = queue_len;
    // 4095 to adjust the descriptor pointer to be 4kb aligned
    // TODO: Instead of relying on root heap allocator implement page allocator
    p_vq->allocated_mem = otrix::alloc(virtq_size + 4095);
    if (nullptr == p_vq->allocated_mem) {
        otrix::free(p_vq);
        return ENOMEM;
    }

    p_vq->desc_table = reinterpret_cast<virtio_descriptor *>(vq_align((uintptr_t)p_vq->allocated_mem));
    p_vq->avail_ring_hdr = (virtio_ring_hdr *)((uint8_t *)p_vq->desc_table + descriptor_table_size);
    p_vq->avail_ring = (uint16_t *)((uint8_t *)p_vq->avail_ring_hdr + sizeof(virtio_ring_hdr));
    p_vq->used_ring_hdr = (virtio_ring_hdr *)((uint8_t *)p_vq->desc_table + vq_align(descriptor_table_size + available_ring_size));
    p_vq->used_ring = (virtio_used_elem *)((uint8_t *)p_vq->used_ring_hdr);

    // TODO: make sure virtqueue allocation happens in 32-bit address space
    write_reg(queue_address, (uintptr_t)p_vq->desc_table & 0xFFFFFFFF);

    *p_out_virtq = p_vq;

    return EOK;
}

error_t virtio_dev::virtq_destroy(virtq *p_vq)
{
    if (nullptr == p_vq) {
        return EINVAL;
    }

    write_reg(queue_select, p_vq->index);
    write_reg(queue_size, 0);
    write_reg(queue_address, 0);

    otrix::free(p_vq->allocated_mem);
    otrix::free(p_vq);

    return EOK;
}

error_t virtio_dev::virtq_add_buffer(virtq *p_vq, uint64_t *p_buffer, uint32_t buffer_size, bool device_writable)
{
    if (nullptr == p_vq || nullptr == p_buffer || 0 == buffer_size) {
        return EINVAL;
    }

    // TODO: Implement free list in the descriptor table
    if (p_vq->num_used_descriptors == p_vq->size) {
        return ENOMEM;
    }

    const int idx = p_vq->num_used_descriptors++;
    virtio_descriptor *p_desc = &p_vq->desc_table[idx];
    p_desc->addr = (uint64_t)p_buffer;
    p_desc->len = buffer_size;
    p_desc->flags = device_writable ? VIRTQ_DESC_F_WRITE : 0;
    p_desc->next = 0;

    p_vq->avail_ring[p_vq->avail_ring_hdr->idx] = idx;
    const int avail_index = (p_vq->avail_ring_hdr->idx + 1) % p_vq->size;

    // Memory barrier to make sure all changes are visible to the device when index is updated
    __sync_synchronize();
    p_vq->avail_ring_hdr->idx = avail_index;

    if (!(p_vq->used_ring_hdr->flags & VIRTQ_USED_F_NO_NOTIFY)) {
        write_reg(queue_notify, p_vq->index);
    }

    return EOK;
}

} // namespace otrix::dev
