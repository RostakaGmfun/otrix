#pragma once

#include <cstdint>
#include <cstddef>
#include <tuple>
#include "common/error.h"

namespace otrix::net
{

class linkif;
class sockbuf;
class arp;

typedef uint32_t ipv4_t;

enum class ipproto_t: uint8_t
{
    unknown = 0x00,
    icmp = 0x01,
    tcp = 0x06,
    udp = 0x11,
};

struct ip_hdr {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t len;
    uint16_t id;
    uint16_t flags_offset;
    uint8_t ttl;
    uint8_t proto;
    uint16_t csum;
    uint32_t saddr;
    uint32_t daddr;
} __attribute__((packed));

static inline ipv4_t make_ipv4(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    return a << 24 | b << 16 | c << 8 | d;
}

static inline uint16_t ip_checksum(const uint8_t *p_data, size_t data_size)
{
    uint32_t csum = 0;
    const uint16_t *hdr_start = reinterpret_cast<const uint16_t *>(p_data);
    for (size_t i = 0; i < data_size / sizeof(uint16_t); i++) {
        csum += *hdr_start;
        hdr_start++;
    }

    if (data_size % sizeof(uint16_t) != 0) {
        csum += *reinterpret_cast<const uint8_t *>(hdr_start);
    }

    return ~((csum & 0xffff) + ((csum & 0xffff0000) >> 16));
}

typedef void (*l3_handler_t)(sockbuf *data, void *ctx);

/**
 * Represents network (IPv4) layer
 */
class ipv4
{
public:
    ipv4(linkif *link, arp *arp, ipv4_t addr, ipv4_t gateway);

    kerror_t write(sockbuf *data, ipv4_t dest, ipproto_t proto, uint64_t timeout_ms);

    size_t headers_size() const;

    ipv4_t get_addr() const
    {
        return addr_;
    }

    kerror_t subscribe_to_rx(net::ipproto_t type, net::l3_handler_t p_handler, void *ctx);

private:
    void process_packet(sockbuf *data);

    linkif *link_;
    arp *arp_;
    ipv4_t addr_;
    ipv4_t gateway_;

    static constexpr auto MAX_RX_HANDLERS = 3;
    std::tuple<net::ipproto_t, net::l3_handler_t, void *> rx_handlers_[MAX_RX_HANDLERS];
};

} // namespace otrix::net
