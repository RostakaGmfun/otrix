#include "net/netif.hpp"
#include "net/linkif.hpp"

namespace otrix::net
{

netif::netif(linkif *link, ipv4_t addr): link_(link), addr_(addr)
{}

struct iphdr {
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

kerror_t netif::write(const void *data, size_t data_size, const ipv4_t &dest, ipproto_t proto, uint64_t timeout_ms)
{
    (void)data;
    (void)data_size;
    (void)dest;
    (void)proto;
    (void)timeout_ms;

    return E_OK;
}

size_t netif::read(void *data, size_t max_size, ipv4_t *dest, ipproto_t *proto, uint64_t timeout_ms)
{
    (void)data;
    (void)max_size;
    (void)dest;
    (void)proto;
    (void)timeout_ms;
    return 0;
}

} // namespace otrix::net
