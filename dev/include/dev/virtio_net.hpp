#pragma once

#include "dev/virtio.hpp"

namespace otrix::dev
{

class virtio_net final: public virtio_dev
{
public:
    virtio_net(pci_device *pci_dev);
    ~virtio_net() override;

protected:
    uint32_t negotiate_features(uint32_t device_features) override;

private:
    virtq *tx_q_;
    virtq *rx_q_;
    virtq *control_q_;
};

} // namespace otrix::dev
