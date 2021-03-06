#include "net/net_task.hpp"
#include "kernel/kthread.hpp"
#include "dev/pci.hpp"
#include "dev/virtio_net.hpp"
#include "otrix/immediate_console.hpp"
#include "net/ipv4.hpp"
#include "net/sockbuf.hpp"
#include "net/arp.hpp"

namespace otrix::net {

static otrix::dev::pci_dev *net_dev_pci_handle;

static void net_task_entry()
{
    otrix::dev::virtio_net net(net_dev_pci_handle);
    immediate_console::print("Net device created:\n");
    net.print_info();
    net::ipv4 ip_layer(&net, net::make_ipv4(20, 0, 0, 2));
    net::arp arp_layer(&net, ip_layer.get_addr());
    arp_layer.announce();
    while (1) {
        immediate_console::print("Sending packet\n");
        uint8_t test_packet[16] = { 0 };
        net::sockbuf buf(ip_layer.headers_size(), test_packet, sizeof(test_packet));
        const auto dest = net::make_ipv4(255, 255, 255, 254);
        kerror_t ret = ip_layer.write(&buf, dest, ipproto_t::udp, KTHREAD_TIMEOUT_INF);
        immediate_console::print("Result: %d\n", ret);
        scheduler::get().sleep(1000);
    }
}

void net_task_start(otrix::dev::pci_dev *net_dev)
{
    net_dev_pci_handle = net_dev;
    kthread *net_task = new kthread(16 * 1024 / sizeof(uint64_t), net_task_entry, "net_task", 2);
    otrix::scheduler::get().add_thread(net_task);
}

} // namespace otrix::net
