#include "net/net_task.hpp"
#include <memory>

#include "kernel/kthread.hpp"
#include "dev/pci.hpp"
#include "dev/virtio_net.hpp"
#include "otrix/immediate_console.hpp"
#include "net/ipv4.hpp"
#include "net/sockbuf.hpp"
#include "net/arp.hpp"
#include "net/icmp.hpp"
#include "net/tcp.hpp"
#include "net/socket.hpp"

namespace otrix::net {

static void tcp_server(tcp *p_tcp, uint16_t port)
{
    std::shared_ptr<socket> srv = std::shared_ptr<socket>(p_tcp->create_socket());
    if (nullptr == srv) {
        immediate_console::print("Failed to create socket\n");
        return;
    }

    kerror_t ret = srv->bind(port);
    if (E_OK != ret) {
        immediate_console::print("Failed to bind socket to %d, err %d\n", port, ret);
        return;
    }

    ret = srv->listen(10);
    if (E_OK != ret) {
        immediate_console::print("Failed to listen on port %d, err %d\n", port, ret);
        return;
    }

    immediate_console::print("Listening for incoming connections on port %d\n", port);

    socket *client = srv->accept();
    immediate_console::print("Accepted connection from %p %08x:%d\n", client, client->get_remote_addr(), client->get_remote_port());
    while (true) {
        char buf[256];
        const size_t recv_ret = client->recv(buf, sizeof(buf));
        if (recv_ret > 0) {
            const size_t send_ret = client->send(buf, recv_ret);
            if (send_ret != recv_ret) {
                immediate_console::print("Failed to send %lu %lu\r\n", send_ret, recv_ret);
            }
        }
    }
    delete client;
}

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

    tcp_server(&tcp_layer, 80);

    scheduler::get().sleep(-1);
}

void net_task_start(otrix::dev::pci_dev *net_dev)
{
    kthread *net_task = new kthread(64 * 1024 / sizeof(uint64_t), net_task_entry, "net_task", 2, net_dev);
    otrix::scheduler::get().add_thread(net_task);
}

} // namespace otrix::net
