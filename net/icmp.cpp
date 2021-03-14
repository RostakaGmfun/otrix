#include "net/icmp.hpp"
#include "net/sockbuf.hpp"
#include "net/ipv4.hpp"
#include "common/utils.h"
#include "otrix/immediate_console.hpp"
#include "arch/asm.h"

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

struct icmp_echo_hdr {
    uint16_t id_seq;
    uint8_t data[];
} __attribute__((packed));

static constexpr auto ICMP_PAYLOAD_SIZE = 64;

icmp::icmp(ipv4 *ip): ip_(ip)
{
    ip_->subscribe_to_rx(ipproto_t::icmp, [] (sockbuf *data, void *ctx) {
                icmp *p_this = (icmp *)ctx;
                p_this->process_packet(data);
            }, this);
}

bool icmp::ping(ipv4_t addr, uint64_t timeout)
{
    ping_req_semaphore_.take(0);
    static int req_id = 1; // TODO: add random
    ping_req_id_ = req_id++;
    ping_req_pending_ = true;
    sockbuf request(ip_->headers_size() + sizeof(icmp_v4_hdr), nullptr, sizeof(icmp_echo_hdr) + ICMP_PAYLOAD_SIZE);
    icmp_echo_hdr *echo_cmd = (icmp_echo_hdr *)request.payload();
    echo_cmd->id_seq = ping_req_id_;
    for (int i = 0; i < ICMP_PAYLOAD_SIZE; i++) {
        echo_cmd->data[i] = i;
    }

    request.add_header(sizeof(icmp_v4_hdr), sockbuf_header_t::icmp);
    icmp_v4_hdr *p_request_hdr = (icmp_v4_hdr *)request.header(sockbuf_header_t::icmp);
    p_request_hdr->type = icmp_type::echo;
    p_request_hdr->csum = 0;
    p_request_hdr->code = 0;

    const size_t csum_size = sizeof(icmp_v4_hdr) + request.payload_size();
    const uint16_t csum_computed = ip_checksum((uint8_t *)p_request_hdr, csum_size);
    p_request_hdr->csum = csum_computed;

    // TODO: decrease timeout when multiple blocking operations are issues
    ip_->write(&request, addr, ipproto_t::icmp, timeout);

    const bool ret = ping_req_semaphore_.take(timeout);
    ping_req_pending_ = false;
    return ret;
}

void icmp::process_packet(sockbuf *data)
{
    if (data->payload_size() < sizeof(ip_hdr)) {
        return;
    }
    data->add_parsed_header(sizeof(icmp_v4_hdr), sockbuf_header_t::icmp);
    const icmp_v4_hdr *p_hdr = (icmp_v4_hdr *)data->header(sockbuf_header_t::icmp);

    switch (p_hdr->type) {
    case echo:
        process_echo_request(data);
        break;
    case echo_reply:
        process_echo_reply(data);
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

void icmp::process_echo_reply(sockbuf *data)
{
    auto flags = arch_irq_save();
    if (!ping_req_pending_) {
        arch_irq_restore(flags);
        return;
    }

    if (data->payload_size() - sizeof(icmp_echo_hdr) != ICMP_PAYLOAD_SIZE) {
        arch_irq_restore(flags);
        return;
    }

    const icmp_echo_hdr *p_echo = (icmp_echo_hdr *)data->payload();
    if ((p_echo->id_seq & 0xFFFF) != ping_req_id_) {
        arch_irq_restore(flags);
        return;
    }

    for (size_t i = 0; i < data->payload_size() - sizeof(icmp_echo_hdr); i++) {
        if (i != p_echo->data[i]) {
            arch_irq_restore(flags);
            return;
        }
    }
    ping_req_semaphore_.give();
    arch_irq_restore(flags);
}

} // namespace otrix::net
