#pragma once

#include "common/error.h"
#include "net/ipv4.hpp"
#include "net/ethernet.hpp"
#include <utility>

namespace otrix::net
{

class linkif;
class sockbuf;

class arp
{
public:
    arp(linkif *p_linkif, ipv4_t ip_addr);

    bool get_ip(const mac_t &mac, ipv4_t *p_ip);

    bool get_mac(ipv4_t addr, mac_t *p_mac);

    void process_packet(sockbuf *data);

    kerror_t announce();

private:
    kerror_t send_reply(ipv4_t addr, const mac_t &target_mac, ipv4_t target_ip);

    static constexpr auto TABLE_SIZE = 4;
    std::pair<ipv4_t, mac_t> table_[TABLE_SIZE];
    linkif *linkif_;
    mac_t addr_;
    ipv4_t ip_addr_;
};

} // namespace otrix::net
