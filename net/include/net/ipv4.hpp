#pragma once

#include <cstdint>
#include <cstddef>
#include "common/error.h"
#include "net/sockbuf.hpp"

namespace otrix::net
{

class linkif;

typedef uint32_t ipv4_t;

enum class ipproto_t: uint8_t
{
    tcp = 0x06,
    udp = 0x11,
};

static inline ipv4_t make_ipv4(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    return a << 24 | b << 16 | c << 8 | d;
}

/**
 * Represents network (IPv4) layer
 */
class ipv4
{
public:
    ipv4(linkif *link, ipv4_t addr);

    kerror_t write(sockbuf *data, ipv4_t dest, ipproto_t proto, uint64_t timeout_ms);
    sockbuf *read(ipv4_t *dest, ipproto_t *proto, uint64_t timeout_ms);

    size_t headers_size() const;

private:
    linkif *link_;
    ipv4_t addr_;
};

} // namespace otrix::net
