#include "net/ipv4.hpp"
#include "net/linkif.hpp"
#include "common/utils.h"
#include "net/sockbuf.hpp"
#include "arch/asm.h"

namespace otrix::net
{

ipv4::ipv4(linkif *link, ipv4_t addr): link_(link), addr_(addr)
{
    link_->subscribe_to_rx(ethertype::ipv4, [] (sockbuf *data, void *ctx) {
                ipv4 *p_this = (ipv4 *)ctx;
                p_this->process_packet(data);
            }, this);
}

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

static constexpr uint8_t VERSION_IHL_IPV4 = (4 << 4) | (sizeof(ip_hdr) / sizeof(uint32_t));

kerror_t ipv4::write(sockbuf *data, ipv4_t dest, ipproto_t proto, uint64_t timeout_ms)
{
    ip_hdr *hdr = reinterpret_cast<ip_hdr *>(data->add_header(sizeof(ip_hdr), sockbuf_header_t::ip));
    hdr->version_ihl = VERSION_IHL_IPV4;
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
    uint32_t csum = 0;
    for (size_t i = 0; i < sizeof(ip_hdr) / sizeof(uint16_t); i++) {
        csum += *hdr_start;
        hdr_start++;
    }
    const uint16_t csum_computed = ~((csum & 0xffff) + ((csum & 0xffff0000) >> 16));

    hdr->csum = htons(csum_computed);
    const mac_t destination = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    return link_->write(data, destination, ethertype::ipv4, timeout_ms);
}

size_t ipv4::headers_size() const
{
    return sizeof(ip_hdr) + link_->headers_size();
}

kerror_t ipv4::subscribe_to_rx(ipproto_t type, l3_handler_t p_handler, void *ctx)
{
    auto flags = arch_irq_save();
    for (int i = 0; i < MAX_RX_HANDLERS; i++) {
        if (net::ipproto_t::unknown == std::get<0>(rx_handlers_[i])) {
            std::get<0>(rx_handlers_[i]) = type;
            std::get<1>(rx_handlers_[i]) = p_handler;
            std::get<2>(rx_handlers_[i]) = ctx;
            arch_irq_restore(flags);
            return E_OK;
        }
    }
    arch_irq_restore(flags);
    return E_NOMEM;
}

void ipv4::process_packet(sockbuf *data)
{
    if (data->payload_size() < sizeof(ip_hdr)) {
        return;
    }
    data->add_parsed_header(sizeof(ip_hdr), sockbuf_header_t::ip);
    const ip_hdr *p_hdr = (ip_hdr *)data->header(sockbuf_header_t::ip);

    uint32_t csum = 0;
    const uint16_t *hdr_start = reinterpret_cast<const uint16_t *>(p_hdr);
    for (size_t i = 0; i < sizeof(ip_hdr) / sizeof(uint16_t); i++) {
        csum += *hdr_start;
        hdr_start++;
    }

    const uint16_t csum_computed = ~((csum & 0xffff) + ((csum & 0xffff0000) >> 16));
    if (csum_computed != 0) {
        return;
    }

    if (p_hdr->version_ihl != VERSION_IHL_IPV4) {
        return;
    }

    const ipv4_t broadcast = make_ipv4(255, 255, 255, 255);
    if (p_hdr->daddr != htonl(addr_) && p_hdr->daddr != htonl(broadcast)) {
        // No routing support
        return;
    }

    const size_t len = ntohs(p_hdr->len) - sizeof(ip_hdr);
    data->set_payload_size(len);

    for (const auto handler : rx_handlers_) {
        if (std::get<0>(handler) == (ipproto_t)p_hdr->proto) {
            std::get<1>(handler)(data, std::get<2>(handler));
        }
    }
}

} // namespace otrix::net
