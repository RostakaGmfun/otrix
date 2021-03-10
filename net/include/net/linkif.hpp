#pragma once

#include <stddef.h>
#include "common/error.h"
#include "net/ethernet.hpp"

namespace otrix::net
{

class sockbuf;

typedef void (*l2_handler_t)(sockbuf *data, void *ctx);

/**
 * Represents link layer
 */
class linkif
{
public:
    virtual void get_mac(mac_t *p_out) = 0;

    virtual kerror_t write(sockbuf *data, const mac_t &dest, ethertype type, uint64_t timeout_ms) = 0;
    virtual kerror_t subscribe_to_rx(ethertype type, l2_handler_t p_handler, void *ctx) = 0;

    virtual size_t headers_size() const = 0;
};

} // namespace otrix::net
