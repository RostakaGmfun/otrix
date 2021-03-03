#pragma once
#include "dev/pci.hpp"

namespace otrix::net {

void net_task_start(otrix::dev::pci_dev *net_dev);

} // namespace otrix::net
