#include "net/ipv4.hpp"
#include "net/linkif.hpp"
#include "common/utils.h"

namespace otrix::net
{

ipv4::ipv4(linkif *link, ipv4_t addr): link_(link), addr_(addr)
{}

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

kerror_t ipv4::write(sockbuf *data, ipv4_t dest, ipproto_t proto, uint64_t timeout_ms)
{
    ip_hdr *hdr = reinterpret_cast<ip_hdr *>(data->add_header(sizeof(ip_hdr), sockbuf_header_t::ip));
    hdr->version_ihl = (4 << 4) | (sizeof(ip_hdr) / sizeof(uint32_t));
    hdr->tos = 0;
    hdr->len = htons(data->size());
    hdr->id = 0; // TODO: segment packets longer than 64k
    hdr->flags_offset = 0;
    hdr->ttl = 64;
    hdr->proto = static_cast<uint8_t>(proto);
    hdr->csum = 0;
    hdr->saddr = htonl(addr_);
    hdr->daddr = htonl(dest);

    const uint16_t *hdr_start = reinterpret_cast<uint16_t *>(hdr);
    uint16_t csum = 0;
    for (size_t i = 0; i < sizeof(ip_hdr) / sizeof(uint16_t); i++) {
        csum += *hdr_start;
        hdr_start++;
    }

    hdr->csum = htons(csum);
    const mac_t destination = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    return link_->write(data, destination, ethertype::ipv4, timeout_ms);
}

sockbuf *ipv4::read(ipv4_t *dest, ipproto_t *proto, uint64_t timeout_ms)
{
    (void)dest;
    (void)proto;
    (void)timeout_ms;
    return 0;
}


size_t ipv4::headers_size() const
{
    return sizeof(ip_hdr) + link_->headers_size();
}

} // namespace otrix::net
