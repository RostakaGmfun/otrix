#include "net/net_task.hpp"
#include "kernel/kthread.hpp"
#include "dev/pci.hpp"
#include "dev/virtio_net.hpp"
#include "otrix/immediate_console.hpp"
#include "net/ipv4.hpp"
#include "net/sockbuf.hpp"
#include "net/arp.hpp"

namespace otrix::net {

static void net_task_entry(void *arg)
{
    otrix::dev::virtio_net net((otrix::dev::pci_dev *)arg);
    immediate_console::print("Net device created:\n");
    net.print_info();
    net::ipv4 ip_layer(&net, net::make_ipv4(20, 0, 0, 2));
    net::arp arp_layer(&net, ip_layer.get_addr());
    arp_layer.announce();
    while (1) {
        scheduler::get().sleep(1000);
    }
}

void net_task_start(otrix::dev::pci_dev *net_dev)
{
    kthread *net_task = new kthread(64 * 1024 / sizeof(uint64_t), net_task_entry, "net_task", 2, net_dev);
    otrix::scheduler::get().add_thread(net_task);
}

} // namespace otrix::net
