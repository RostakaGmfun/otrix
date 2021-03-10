#include "net/arp.hpp"

#include <cstring>
#include "net/ethernet.hpp"
#include "net/linkif.hpp"
#include "net/sockbuf.hpp"
#include "common/utils.h"

namespace otrix::net
{

static constexpr uint16_t ARP_REQUEST = 1;
static constexpr uint16_t ARP_REPLY = 2;

struct arp_header_ipv4
{
    uint16_t hardware_type;
    uint16_t protocol_type;
    uint8_t hln;
    uint8_t pln;
    uint16_t operation;
    mac_t sender_mac;
    ipv4_t sender_ip;
    mac_t target_mac;
    ipv4_t target_ip;
} __attribute__((packed));

arp::arp(linkif *p_linkif, ipv4_t ip_addr): linkif_(p_linkif), ip_addr_(ip_addr)
{
    linkif_->get_mac(&addr_);
    linkif_->subscribe_to_rx(ethertype::arp, [] (sockbuf *data, void *ctx) {
                arp *p_this = (arp *)ctx;
                p_this->process_packet(data);
            }, this);
}

bool arp::get_ip(const mac_t &mac, ipv4_t *p_ip)
{
    for (const auto &entry : table_) {
        if (entry.second == mac) {
            *p_ip = entry.first;
            return true;
        }
    }
    return false;
}

bool arp::get_mac(ipv4_t addr, mac_t *p_mac)
{
    for (const auto &entry : table_) {
        if (entry.first == addr) {
            memcpy(p_mac, &entry.second, sizeof(entry.second));
            return true;
        }
    }
    return false;
}

void arp::process_packet(sockbuf *data)
{
    if (data->payload_size() < sizeof(arp_header_ipv4)) {
        return;
    }
    const arp_header_ipv4 *p_arp = (arp_header_ipv4 *)data->payload();

    if (p_arp->operation == ntohs(ARP_REQUEST) && ntohl(p_arp->target_ip) == ip_addr_) {
        send_reply(ip_addr_, p_arp->sender_mac, p_arp->sender_ip);
    }

    bool empty_found = false;
    for (auto &entry : table_) {
        if (0 == entry.first) {
            empty_found = true;
            entry.first = ntohl(p_arp->sender_ip);
            memcpy(entry.second, p_arp->sender_mac, sizeof(entry.second));
            break;
        }
    }

    if (!empty_found) {
        // TODO: evict in a smarter way
        table_[0].first = ntohl(p_arp->sender_ip);
        memcpy(table_[0].second, p_arp->sender_mac, sizeof(table_[0].second));
    }
}

kerror_t arp::announce()
{
    mac_t target = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    return send_reply(ip_addr_, target, make_ipv4(255, 255, 255, 255));
}

kerror_t arp::send_reply(ipv4_t addr, const mac_t &target_mac, ipv4_t target_ip)
{
    arp_header_ipv4 payload;
    payload.hardware_type = htons(1); // ethernet
    payload.protocol_type = htons((uint16_t)ethertype::ipv4);
    payload.hln = sizeof(mac_t);
    payload.pln = sizeof(ipv4_t);
    payload.operation = htons(ARP_REPLY);
    linkif_->get_mac(&payload.sender_mac);
    payload.sender_ip = htonl(addr);
    memcpy(payload.target_mac, target_mac, sizeof(target_mac));
    payload.target_ip = htonl(target_ip);
    sockbuf packet(linkif_->headers_size(), (uint8_t *)&payload, sizeof(payload));
    return linkif_->write(&packet, payload.target_mac, ethertype::arp, -1);
}


} // namespace otrix::net
