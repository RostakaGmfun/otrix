#pragma once

#include <cstdint>
#include "net/linkif.hpp"

namespace otrix::net
{

typedef uint32_t ipv4;

/**
 * Represents network (IPv4) layer
 */
class netif
{
public:
    netif(linkif *link, ipv4 addr);

private:
    linkif *link_;
    ipv4 addr_;
};

} // namespace otrix::net
