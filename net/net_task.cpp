#include "net/net_task.hpp"
#include "kernel/kthread.hpp"
#include "dev/pci.hpp"
#include "dev/virtio_net.hpp"
#include "otrix/immediate_console.hpp"

namespace otrix::net {

static otrix::dev::pci_dev *net_dev_pci_handle;

static void net_task_entry()
{
    otrix::dev::virtio_net net(net_dev_pci_handle);
    immediate_console::print("Net device created:\n");
    net.print_info();
    while (1) {
        immediate_console::print("Sending packet\n");
        uint8_t test_packet[16] = { 0 };
        net::mac_t destination = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
        kerror_t ret = net.write(test_packet, sizeof(test_packet), destination, ethertype::unknown, KTHREAD_TIMEOUT_INF);
        immediate_console::print("Result: %d\n", ret);
        scheduler::get().sleep(5000);
    }
}

void net_task_start(otrix::dev::pci_dev *net_dev)
{
    net_dev_pci_handle = net_dev;
    kthread *net_task = new kthread(16 * 1024 / sizeof(uint64_t), net_task_entry, "net_task", 2);
    otrix::scheduler::get().add_thread(net_task);
}

} // namespace otrix::net
