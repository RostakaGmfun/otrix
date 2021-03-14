#include "net/icmp.hpp"
#include "net/sockbuf.hpp"
#include "net/ipv4.hpp"
#include "common/utils.h"
#include "otrix/immediate_console.hpp"

namespace otrix::net
{

enum icmp_type: uint8_t {
    echo_reply = 0x00,
    destination_unreachable = 0x03,
    echo = 0x08,
};

struct icmp_v4_hdr {
    uint8_t type;
    uint8_t code;
    uint16_t csum;
} __attribute__((packed));

icmp::icmp(ipv4 *ip): ip_(ip)
{
    ip_->subscribe_to_rx(ipproto_t::icmp, [] (sockbuf *data, void *ctx) {
                icmp *p_this = (icmp *)ctx;
                p_this->process_packet(data);
            }, this);
}

void icmp::process_packet(sockbuf *data)
{
    if (data->payload_size() < sizeof(ip_hdr)) {
        return;
    }
    data->add_parsed_header(sizeof(icmp_v4_hdr), sockbuf_header_t::icmp);
    const icmp_v4_hdr *p_hdr = (icmp_v4_hdr *)data->header(sockbuf_header_t::icmp);

    immediate_console::print("Got ICMP packet of type %d\n", p_hdr->type);

    switch (p_hdr->type) {
    case echo:
        process_echo_request(data);
        break;
    default:
        break;
    }
}

void icmp::process_echo_request(sockbuf *data)
{
    const ip_hdr *p_ip_hdr = (const ip_hdr *)data->header(sockbuf_header_t::ip);

    sockbuf reply(ip_->headers_size() + sizeof(icmp_v4_hdr), data->payload(), data->payload_size());
    reply.add_header(sizeof(icmp_v4_hdr), sockbuf_header_t::icmp);
    icmp_v4_hdr *p_reply_hdr = (icmp_v4_hdr *)reply.header(sockbuf_header_t::icmp);
    p_reply_hdr->type = icmp_type::echo_reply;
    p_reply_hdr->csum = 0;
    p_reply_hdr->code = 0;
    const size_t csum_size = sizeof(icmp_v4_hdr) + reply.payload_size();
    const uint16_t csum_computed = ip_checksum((uint8_t *)p_reply_hdr, csum_size);
    p_reply_hdr->csum = csum_computed;

    ip_->write(&reply, ntohl(p_ip_hdr->saddr), ipproto_t::icmp, -1);
}

} // namespace otrix::net
