#include "net/ipv4.hpp"
#include "net/linkif.hpp"
#include "net/arp.hpp"
#include "common/utils.h"
#include "net/sockbuf.hpp"
#include "arch/asm.h"
#include "otrix/immediate_console.hpp"

namespace otrix::net
{

ipv4::ipv4(linkif *link, arp *arp, ipv4_t addr, ipv4_t gateway): link_(link), arp_(arp), addr_(addr), gateway_(gateway)
{
    link_->subscribe_to_rx(ethertype::ipv4, [] (sockbuf *data, void *ctx) {
                ipv4 *p_this = (ipv4 *)ctx;
                p_this->process_packet(data);
            }, this);
}

static constexpr uint8_t VERSION_IHL_IPV4 = (4 << 4) | (sizeof(ip_hdr) / sizeof(uint32_t));

kerror_t ipv4::write(sockbuf *data, ipv4_t dest, ipproto_t proto, uint64_t timeout_ms)
{
    mac_t destination;
    if (!arp_->resolve(gateway_, &destination, timeout_ms)) {
        return E_UNREACHABLE;
    }

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

    const uint16_t csum_computed = ip_checksum((const uint8_t *)hdr, sizeof(ip_hdr));

    hdr->csum = csum_computed;
    immediate_console::print("Sending packet of type %d to %08x\n", proto, dest);
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

    const uint16_t csum_computed = ip_checksum((const uint8_t *)p_hdr, sizeof(ip_hdr));
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

    immediate_console::print("Got IPv4 packet of type %d\n", p_hdr->proto);

    for (const auto handler : rx_handlers_) {
        if (std::get<0>(handler) == (ipproto_t)p_hdr->proto) {
            std::get<1>(handler)(data, std::get<2>(handler));
        }
    }
}

} // namespace otrix::net
