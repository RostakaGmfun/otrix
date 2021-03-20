#include "net/tcp.hpp"
#include <functional>
#include "common/utils.h"
#include "net/sockbuf.hpp"
#include "net/tcp_socket.hpp"
#include "otrix/immediate_console.hpp"

namespace otrix::net
{

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
                          listen_sockets_(TCP_LISTEN_TABLE_SIZE, [] (const socket_id &key) -> size_t { return key.host_port; }),
                          connected_sockets_(TCP_CONNECTED_TABLE_SIZE, socket_id::hash_func)
{
    ip_layer_->subscribe_to_rx(ipproto_t::tcp, [] (sockbuf *data, void *ctx) {
                tcp *p_this = (tcp *)ctx;
                p_this->process_packet(data);
            }, this);
}

socket *tcp::create_socket()
{
    return new tcp_socket(this);
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

kerror_t tcp::bind_socket(tcp_socket *sock, uint16_t port)
{
    socket_id id{port, 0, 0};
    auto hm_node = listen_sockets_.find(id);
    if (nullptr != hm_node) {
        return E_ADDRINUSE;
    }

    sock->node()->hm_node.key = id;

    listen_sockets_.insert(&sock->node()->hm_node);

    return E_OK;
}

kerror_t tcp::unbind_socket(tcp_socket *sock)
{
    listen_sockets_.remove(&sock->node()->hm_node);
    return E_OK;
}

kerror_t tcp::add_connected_socket(tcp_socket *sock, socket_id id)
{
    auto hm_node = listen_sockets_.find(id);
    if (nullptr != hm_node) {
        return E_ADDRINUSE;
    }

    sock->node()->hm_node.key = id;

    connected_sockets_.insert(&sock->node()->hm_node);

    return E_OK;
}

kerror_t tcp::remove_connected_socket(tcp_socket *sock)
{
    connected_sockets_.remove(&sock->node()->hm_node);
    return E_OK;
}

void tcp::process_packet(sockbuf *data)
{
    data->add_parsed_header(tcp_header_size(data), sockbuf_header_t::tcp);
    const tcp_header *p_hdr = (tcp_header *)data->header(sockbuf_header_t::tcp);
    hash_map_node<socket_id> *hm_node = nullptr;
    const ip_hdr *p_ip_hdr = (ip_hdr *)data->header(sockbuf_header_t::ip);
    socket_id connected_id{ntohs(p_hdr->dest_port), ntohs(p_hdr->source_port), ntohl(p_ip_hdr->saddr)};
    auto connected_node = connected_sockets_.find(connected_id);
    if (p_hdr->flags & TCP_FLAG_SYN || nullptr == connected_node) {
        // Handle incoming connection request
        socket_id listen_id{ntohs(p_hdr->dest_port), 0, 0};
        hm_node = listen_sockets_.find(listen_id);
    } else {
        // Handle active connection, if any
        hm_node = connected_node;
    }

    if (nullptr == hm_node) {
        send_reply(data, TCP_FLAG_RST | TCP_FLAG_ACK);
    } else {
        tcp_socket *sock = container_of(hm_node, tcp_socket::node_t, hm_node)->p_socket;
        sock->process_packet(data);
    }
}

kerror_t tcp::send_reply(const sockbuf *reply_to, uint8_t tcp_flags)
{
    const tcp_header *p_in_hdr = (tcp_header *)reply_to->header(sockbuf_header_t::tcp);
    const ip_hdr *p_ip_hdr = (ip_hdr *)reply_to->header(sockbuf_header_t::ip);

    sockbuf *reply = new sockbuf(ip_layer_->headers_size() + sizeof(tcp_header), nullptr, 0);
    tcp_header *p_tcp_hdr = (tcp_header *)reply->add_header(sizeof(tcp_header), sockbuf_header_t::tcp);
    p_tcp_hdr->source_port = p_in_hdr->dest_port;
    p_tcp_hdr->dest_port = p_in_hdr->source_port;
    p_tcp_hdr->seq = p_in_hdr->flags & TCP_FLAG_ACK ? p_in_hdr->ack : 0;
    p_tcp_hdr->ack = htonl(ntohl(p_in_hdr->seq) + reply_to->payload_size() + 1);
    p_tcp_hdr->header_len = (sizeof(tcp_header) / sizeof(uint32_t)) << 4;
    p_tcp_hdr->flags = tcp_flags;
    p_tcp_hdr->window_size = 0;
    p_tcp_hdr->csum = 0;
    p_tcp_hdr->urp = 0;

    const uint16_t csum = tcp_checksum(reply, p_ip_hdr->saddr);
    p_tcp_hdr->csum = csum;
    return ip_layer_->write(reply, ntohl(p_ip_hdr->saddr), ipproto_t::tcp, -1);
}

kerror_t tcp::send(ipv4_t remote_addr, sockbuf *buf)
{
    tcp_header *p_tcp_hdr = (tcp_header *)buf->header(sockbuf_header_t::tcp);
    const uint16_t csum = tcp_checksum(buf, htonl(remote_addr));
    p_tcp_hdr->csum = csum;
    return ip_layer_->write(buf, remote_addr, ipproto_t::tcp, -1);
}

size_t tcp::headers_size() const
{
    return ip_layer_->headers_size() + sizeof(tcp_header);
}

} // namespace otrix::net
