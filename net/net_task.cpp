#include "net/net_task.hpp"
#include "kernel/kthread.hpp"
#include "dev/pci.hpp"
#include "dev/virtio_net.hpp"
#include "otrix/immediate_console.hpp"
#include "net/ipv4.hpp"
#include "net/sockbuf.hpp"
#include "net/arp.hpp"
#include "net/icmp.hpp"
#include "net/tcp.hpp"

namespace otrix::net {

static void net_task_entry(void *arg)
{
    otrix::dev::virtio_net net((otrix::dev::pci_dev *)arg);
    immediate_console::print("Net device created:\n");
    net.print_info();
    const auto address = net::make_ipv4(20, 0, 0, 2);
    const auto gateway = net::make_ipv4(20, 0, 0, 1);
    net::arp arp_layer(&net, address);
    net::ipv4 ip_layer(&net, &arp_layer, address, gateway);
    net::icmp icmp_layer(&ip_layer);
    net::tcp tcp_layer(&ip_layer);
    arp_layer.announce();
    arp_layer.send_request(gateway);
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
