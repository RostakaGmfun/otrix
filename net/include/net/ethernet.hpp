#pragma once

#include <cstdint>

namespace otrix::net
{

enum class ethertype: uint16_t {
    unknown = 0,
    ipv4 = 0x800,
    arp = 0x0806,
};

struct ethernet_hdr
{
    uint8_t dmac[6];
    uint8_t smac[6];
    uint16_t ethertype;
} __attribute__((packed));

typedef uint8_t mac_t[6];

} // namespace otrix::net
