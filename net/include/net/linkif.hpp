#pragma once

#include <stddef.h>
#include "common/error.h"
#include "net/ethernet.hpp"
#include "net/sockbuf.hpp"

namespace otrix::net
{

typedef uint8_t mac_t[6];

/**
 * Represents link layer
 */
class linkif
{
public:
    virtual void get_mac(mac_t *p_out) = 0;

    virtual kerror_t write(sockbuf *data, const mac_t &dest, ethertype type, uint64_t timeout_ms) = 0;
    virtual sockbuf *read(mac_t *src, ethertype *type, uint64_t timeout_ms) = 0;

    virtual size_t headers_size() const = 0;
};

} // namespace otrix::net
