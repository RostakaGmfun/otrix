#pragma once

#include <cstdint>
#include <cstddef>
#include <tuple>
#include "common/error.h"

namespace otrix::net
{

class linkif;
class sockbuf;

typedef uint32_t ipv4_t;

enum class ipproto_t: uint8_t
{
    unknown = 0x00,
    tcp = 0x06,
    udp = 0x11,
};

static inline ipv4_t make_ipv4(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    return a << 24 | b << 16 | c << 8 | d;
}

typedef void (*l3_handler_t)(sockbuf *data, void *ctx);

/**
 * Represents network (IPv4) layer
 */
class ipv4
{
public:
    ipv4(linkif *link, ipv4_t addr);

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
    ipv4_t addr_;

    static constexpr auto MAX_RX_HANDLERS = 2;
    std::tuple<net::ipproto_t, net::l3_handler_t, void *> rx_handlers_[MAX_RX_HANDLERS];
};

} // namespace otrix::net
