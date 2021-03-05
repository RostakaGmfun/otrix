#pragma once

#include <cstdint>
#include <cstddef>
#include "common/error.h"

namespace otrix::net
{

class linkif;

typedef uint32_t ipv4_t;

enum class ipproto_t: uint8_t
{
    tcp = 0x06,
    udp = 0x10,
};

/**
 * Represents network (IPv4) layer
 */
class netif
{
public:
    netif(linkif *link, ipv4_t addr);

    kerror_t write(const void *data, size_t data_size, const ipv4_t &dest, ipproto_t proto, uint64_t timeout_ms);
    size_t read(void *data, size_t max_size, ipv4_t *dest, ipproto_t *proto, uint64_t timeout_ms);

private:
    linkif *link_;
    ipv4_t addr_;
};

} // namespace otrix::net
