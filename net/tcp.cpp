#include "net/tcp.hpp"
#include <functional>
#include "net/sockbuf.hpp"
#include "common/utils.h"

#define TCP_CONG_WND_REDUCED (1 << 7)
#define TCP_ECN_ECHO         (1 << 6)
#define TCP_URG              (1 << 5)
#define TCP_ACK              (1 << 4)
#define TCP_PSH              (1 << 3)
#define TCP_RST              (1 << 2)
#define TCP_SYN              (1 << 1)
#define TCP_FIN              (1 << 0)

namespace otrix::net
{

struct tcp_header
{
    uint16_t source_port;
    uint16_t dest_port;
    uint32_t seq;
    uint32_t ack;
    uint8_t header_len;
    uint8_t flags;
    uint16_t window_size;
    uint16_t csum;
    uint16_t urp;
} __attribute__((packed));

struct ipv4_pseudo_header
{
    uint32_t source_addr;
    uint32_t dest_addr;
    uint8_t zero;
    uint8_t proto;
    uint16_t tcp_length;
} __attribute__((packed));

static size_t tcp_header_size(const sockbuf *buf)
{
    const tcp_header *p_hdr = (const tcp_header *)buf->payload();
    return (p_hdr->header_len >> 4) * sizeof(uint32_t);
}

tcp::tcp(ipv4 *ip_layer): ip_layer_(ip_layer),
                          listen_sockets_(TCP_LISTEN_TABLE_SIZE, socket_id_hash_func),
                          connected_sockets_(TCP_CONNECTED_TABLE_SIZE, socket_id_hash_func)
{
    ip_layer_->subscribe_to_rx(ipproto_t::tcp, [] (sockbuf *data, void *ctx) {
                tcp *p_this = (tcp *)ctx;
                p_this->process_packet(data);
            }, this);
}

socket *tcp::create_socket()
{
    return nullptr;
}

void tcp::process_packet(sockbuf *data)
{
    data->add_parsed_header(tcp_header_size(data), sockbuf_header_t::tcp);
    const tcp_header *p_hdr = (tcp_header *)data->header(sockbuf_header_t::tcp);
    hash_map_node<socket_id> *hm_node = nullptr;
    if (p_hdr->flags & TCP_SYN) {
        // Handle incoming connection request
        socket_id id{ntohs(p_hdr->dest_port), 0, 0};
        hm_node = listen_sockets_.find(id);
    } else {
        const ip_hdr *p_ip_hdr = (ip_hdr *)data->header(sockbuf_header_t::ip);
        // Handle active connection, if any
        socket_id id{ntohs(p_hdr->dest_port), ntohs(p_hdr->source_port), ntohl(p_ip_hdr->saddr)};
        hm_node = connected_sockets_.find(id);
    }

    if (nullptr == hm_node) {
        send_rst_reply(data);
    } else {
        tcp_socket *sock = container_of(hm_node, tcp_socket::node_t, hm_node)->p_socket;
        sock->process_packet(data);
    }
}

uint16_t tcp::tcp_checksum(const sockbuf *buf, ipv4_t dest_address_network_order)
{
    ipv4_pseudo_header pseudo_ip;
    pseudo_ip.source_addr = htonl(ip_layer_->get_addr());
    pseudo_ip.dest_addr = dest_address_network_order;
    pseudo_ip.zero = 0;
    pseudo_ip.proto = (uint8_t)ipproto_t::tcp;
    pseudo_ip.tcp_length = ntohs(buf->payload_size() + sizeof(tcp_header));
    const uint16_t *ptr = (uint16_t *)&pseudo_ip;
    uint32_t initial_csum = 0;
    for (size_t i = 0; i < sizeof(pseudo_ip) / sizeof(uint16_t); i++) {
        initial_csum += *ptr;
        ptr++;
    }
    return ip_checksum(buf->header(sockbuf_header_t::tcp),
            sizeof(tcp_header) + buf->payload_size(), initial_csum);
}

void tcp::send_rst_reply(sockbuf *data)
{
    const tcp_header *p_in_hdr = (tcp_header *)data->header(sockbuf_header_t::tcp);
    const ip_hdr *p_ip_hdr = (ip_hdr *)data->header(sockbuf_header_t::ip);

    sockbuf reply(ip_layer_->headers_size() + sizeof(tcp_header), nullptr, 0);
    tcp_header *p_tcp_hdr = (tcp_header *)reply.add_header(sizeof(tcp_header), sockbuf_header_t::tcp);
    p_tcp_hdr->source_port = p_in_hdr->dest_port;
    p_tcp_hdr->dest_port = p_in_hdr->source_port;
    p_tcp_hdr->seq = p_in_hdr->flags & TCP_ACK ? p_in_hdr->ack : 0;
    p_tcp_hdr->ack = htonl(ntohl(p_in_hdr->seq) + data->payload_size() + 1);
    p_tcp_hdr->header_len = (sizeof(tcp_header) / sizeof(uint32_t)) << 4;
    p_tcp_hdr->flags = TCP_RST | TCP_ACK;
    p_tcp_hdr->window_size = 0;
    p_tcp_hdr->csum = 0;
    p_tcp_hdr->urp = 0;

    const uint16_t csum = tcp_checksum(&reply, p_ip_hdr->saddr);
    p_tcp_hdr->csum = csum;
    ip_layer_->write(&reply, ntohl(p_ip_hdr->saddr), ipproto_t::tcp, -1);
}

size_t tcp::socket_id_hash_func(const tcp::socket_id &key)
{
    size_t ret = std::hash<uint16_t>{}(key.host_port);
    ret ^= std::hash<uint16_t>{}(key.remote_port) + 0x9e3779b9 + (ret<<6) + (ret>>2);
    ret ^= std::hash<uint32_t>{}(key.remote_ip) + 0x9e3779b9 + (ret<<6) + (ret>>2);
    return ret;
}

} // namespace otrix::net
