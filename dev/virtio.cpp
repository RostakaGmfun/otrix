#include "dev/virtio.hpp"

#include <algorithm>

#include "arch/irq_manager.hpp"
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

virtio_dev::virtio_dev(pci_device *pci_dev): pci_dev_(pci_dev), virtqueues_(nullptr)
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
        return pci_dev_read32(pci_dev_, pci_dev_->BAR[0] + reg);
    case queue_size:
    case queue_select:
    case queue_notify:
        return pci_dev_read16(pci_dev_, pci_dev_->BAR[0] + reg);
    case device_status:
    case isr_status:
        return pci_dev_read8(pci_dev_, pci_dev_->BAR[0] + reg);
    case config_msix_vector:
    case queue_msix_vector:
        return pci_dev_read16(pci_dev_, pci_dev_->BAR[0] + reg);
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
        return pci_dev_write32(pci_dev_, pci_dev_->BAR[0] + reg, value);
    case queue_size:
    case queue_select:
    case queue_notify:
        return pci_dev_write16(pci_dev_, pci_dev_->BAR[0] + reg, value);
    case device_status:
    case isr_status:
        return pci_dev_write8(pci_dev_, pci_dev_->BAR[0] + reg, value);
    case config_msix_vector:
    case queue_msix_vector:
        return pci_dev_write16(pci_dev_, pci_dev_->BAR[0] + reg, value);
    }
}

error_t virtio_dev::create_virtqueue(uint16_t index, uint16_t *p_queue_size)
{
    write_reg(queue_select, index);
    const uint16_t max_queue_size = read_reg(queue_size);
    if (0 == max_queue_size) {
        return ENODEV;
    }

    const uint16_t queue_len = std::min(*p_queue_size, max_queue_size);
    write_reg(queue_size, queue_len);
    *p_queue_size = queue_len;

    const size_t descriptor_table_size = sizeof(virtio_descriptor) * queue_len;
    const size_t available_ring_size = sizeof(virtio_ring_hdr) + sizeof(uint16_t) * queue_len;
    const size_t used_ring_size = sizeof(virtio_ring_hdr) + sizeof(virtio_used_elem) * queue_len;

    auto vq_align = [] (auto in) {
        return (in + 4095) & ~4095;
    };

    const size_t virtq_size = vq_align(descriptor_table_size + available_ring_size) + vq_align(used_ring_size);
    (void)virtq_size;

    virtqueue *p_vq = (virtqueue *)otrix::alloc(sizeof(virtqueue));
    if (nullptr == p_vq) {
        return ENOMEM;
    }
    intrusive_list_init(&p_vq->list_node);
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

    write_reg(queue_address, p_vq->desc_table);

    if (nullptr == virtqueues_) {
        virtqueues_ = &p_vq->list_node;
    } else {
        intrusive_list_push_back(virtqueues_, &p_vq->list_node);
    }

    return EOK;
}

} // namespace otrix::dev
