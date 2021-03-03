#pragma once

#include <stddef.h>
#include "common/error.h"
#include "net/ethernet.hpp"

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

    virtual kerror_t write(const void *data, size_t data_size, const mac_t &dest, ethertype type, uint64_t timeout_ms) = 0;
    virtual size_t read(void *data, size_t max_size, mac_t *src, ethertype *type, uint64_t timeout_ms) = 0;
};

} // namespace otrix::net
