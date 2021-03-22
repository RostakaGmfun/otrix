#include "net/tcp_socket.hpp"

#include <climits>
#include "net/tcp.hpp"
#include "net/socket.hpp"
#include "net/sockbuf.hpp"
#include "kernel/msgq.hpp"
#include "common/utils.h"
#include "arch/asm.h"

namespace otrix::net
{

tcp_socket::tcp_socket(tcp *tcp_layer): syn_cache_(nullptr), node_(this), tcp_layer_(tcp_layer),
                                        port_(INVALID_PORT), state_(TCP_STATE_CLOSED),
                                        listen_backlog_(nullptr), seq_(0), ack_(0),
                                        recv_window_size_(0), recv_window_used_(0),
                                        recv_skb_(nullptr), recv_skb_payload_offset_(0)
{

}

tcp_socket::~tcp_socket()
{
    tcp_layer_->unbind_socket(this);
    tcp_layer_->remove_connected_socket(this);
}

size_t tcp_socket::send(const void *data, size_t data_size)
{
    (void)data;
    (void)data_size;
    return E_NOIMPL;
}

size_t tcp_socket::recv(void *data, size_t data_size)
{
    size_t received = 0;
    while (received != data_size) {
        if (nullptr == recv_skb_) {
            recv_waitq_.wait();
        }
        auto flags = arch_irq_save();
        sockbuf *buf = container_of(recv_skb_, sockbuf::node_t, list_node)->p_skb;
        const size_t to_copy = std::min(data_size - received, buf->payload_size() - recv_skb_payload_offset_);
        memcpy((char *)data + received, buf->payload() + recv_skb_payload_offset_, to_copy);
        received += to_copy;
        recv_skb_payload_offset_ += to_copy;
        recv_window_used_ -= to_copy;
        if (recv_skb_payload_offset_ == buf->payload_size()) {
            const tcp_header *p_tcp_hdr = (tcp_header *)buf->header(sockbuf_header_t::tcp);
            const bool push_received = p_tcp_hdr->flags & TCP_FLAG_PSH;
            recv_skb_ = intrusive_list_delete(recv_skb_, recv_skb_);
            recv_skb_payload_offset_ = 0;
            delete buf;
            if (push_received) {
                // Push received, return data to the application immediately
                arch_irq_restore(flags);
                break;
            }
        }
        arch_irq_restore(flags);
    }
    return received;
}

kerror_t tcp_socket::connect(ipv4_t remote_addr, ipv4_t remote_port)
{
    (void)remote_addr;
    (void)remote_port;
    return E_NOIMPL;
}

kerror_t tcp_socket::bind(uint16_t port)
{
    kerror_t ret = tcp_layer_->bind_socket(this, port);
    if (E_OK == ret) {
        port_ = port;
    }
    return ret;
}

kerror_t tcp_socket::listen(size_t backlog_size)
{
    if (INVALID_PORT == port_) {
        return E_ADDRINUSE;
    }
    if (nullptr != listen_backlog_) {
        delete listen_backlog_;
    }
    listen_backlog_ = new msgq(backlog_size, sizeof(socket *));
    if (nullptr != syn_cache_) {
        delete syn_cache_;
    }
    syn_cache_ = new pooled_hash_map<tcp::socket_id, syn_cache_entry>(backlog_size,
            SYN_CACHE_TABLE_SIZE, tcp::socket_id::hash_func);
    state_ = TCP_STATE_LISTEN;
    return E_OK;
}

socket *tcp_socket::accept(uint64_t timeout_ms)
{
    if (nullptr == listen_backlog_ || state_ != TCP_STATE_LISTEN) {
        return nullptr;
    }
    socket *accepted_socket = nullptr;
    listen_backlog_->read(&accepted_socket, timeout_ms);
    return accepted_socket;
}

kerror_t tcp_socket::shutdown()
{
    return E_NOIMPL;
}

void tcp_socket::process_packet(sockbuf *data)
{
    switch (state_) {
    case TCP_STATE_LISTEN:
        handle_listen_state(data);
        break;
    case TCP_STATE_ESTABLISHED:
        handle_established_state(data);
        break;
    default:
        break;
    }
}

kerror_t tcp_socket::accept_connection(sockbuf *data)
{
    const ip_hdr *p_ip_hdr = (ip_hdr *)data->header(sockbuf_header_t::ip);
    const tcp_header *p_tcp_hdr = (tcp_header *)data->header(sockbuf_header_t::tcp);
    tcp::socket_id id{(uint16_t)port_, ntohs(p_tcp_hdr->source_port), ntohl(p_ip_hdr->saddr)};
    auto node = syn_cache_->find(id);
    if (nullptr == node) {
        // TODO: Wireshark complains about wrong ACK value here
        tcp_layer_->send_reply(data, TCP_FLAG_ACK | TCP_FLAG_RST);
        return E_OK;
    }
    if (listen_backlog_->full())
    {
        return E_NOMEM;
    }

    syn_cache_->remove(node);
    // TODO: overload constructor for connected socket
    tcp_socket *new_conn = new tcp_socket(tcp_layer_);
    new_conn->port_ = port_;
    new_conn->state_ = TCP_STATE_ESTABLISHED;
    new_conn->node_.hm_node.key = id;
    new_conn->remote_addr_ = ntohl(p_ip_hdr->saddr);
    new_conn->remote_port_ = ntohs(p_tcp_hdr->source_port);
    new_conn->seq_ = ntohl(p_tcp_hdr->ack); // TODO: verify this
    new_conn->ack_ = ntohl(p_tcp_hdr->seq);
    new_conn->recv_window_size_ = TCP_INITIAL_WINDOW_SIZE;
    new_conn->recv_window_used_ = data->payload_size();
    tcp_layer_->add_connected_socket(new_conn, id);

    if (data->payload_size() > 0) {
        new_conn->handle_segment(data);
    }
    return listen_backlog_->write(&new_conn) ? E_OK : E_NOMEM;
}

kerror_t tcp_socket::handle_syn(const sockbuf *data)
{
    const ip_hdr *p_ip_hdr = (ip_hdr *)data->header(sockbuf_header_t::ip);
    const tcp_header *p_tcp_hdr = (tcp_header *)data->header(sockbuf_header_t::tcp);
    tcp::socket_id id{(uint16_t)port_, ntohs(p_tcp_hdr->source_port), ntohl(p_ip_hdr->saddr)};

    auto syn_entry = syn_cache_->find(id);
    if (nullptr != syn_entry) {
        // SYN (likely) retransmitted
        // Check incoming seq number and send SYN+ACK again
        auto *entry = syn_cache_->to_entry(syn_entry);
        if (ntohl(p_tcp_hdr->seq) == entry->value.remote_isn) {
            return send_syn_ack(data, entry->value.isn);
        } else {
            return tcp_layer_->send_reply(data, TCP_FLAG_ACK | TCP_FLAG_RST);
        }
    }

    const uint32_t isn = generate_isn();

    if (nullptr == syn_cache_->insert(id, syn_cache_entry{isn, ntohl(p_tcp_hdr->seq)})) {
        return E_NOMEM;
    }

    // TODO: retransmit on timer and delete syncache entry on timeout
    return send_syn_ack(data, isn);
}

kerror_t tcp_socket::handle_segment(sockbuf *data)
{
    const tcp_header *p_in_hdr = (tcp_header *)data->header(sockbuf_header_t::tcp);
    if (ntohl(p_in_hdr->seq) < ack_) {
        // Received older segment --
        // Re-send emtpy ACK packet with up to date ACK value to inform the remote host.
        return send_packet(TCP_FLAG_ACK);

    }
    if (recv_window_size_ - recv_window_used_ < data->payload_size()) {
        // Window is full, ignore
        return E_NOMEM;
    }
    recv_window_used_ += data->payload_size();
    ack_ += data->payload_size();
    if (data->payload_size() > 0) {
        // Zero-copy receive in action:
        // Move underlying buffer ownership from incoming skb to internal copy
        // which will be freed when application
        // reads data from the payload.
        // TODO: Use copy semantics for smaller payloads
        sockbuf *sk_copy = new sockbuf(std::move(*data));
        recv_skb_ = intrusive_list_push_back(recv_skb_, &sk_copy->node()->list_node);
        recv_waitq_.notify_one();
    }
    // TODO: implement smarter ACKing algorithm
    return send_packet(TCP_FLAG_ACK);
}

void tcp_socket::handle_listen_state(sockbuf *data)
{
    const tcp_header *p_hdr = (tcp_header *)data->header(sockbuf_header_t::tcp);

    if ((p_hdr->flags & TCP_FLAG_SYN) == TCP_FLAG_SYN) {
        handle_syn(data);
    } else if ((p_hdr->flags & TCP_FLAG_ACK) == TCP_FLAG_ACK) {
        accept_connection(data);
    } else {
        tcp_layer_->send_reply(data, TCP_FLAG_RST | TCP_FLAG_ACK);
    }
}

void tcp_socket::handle_established_state(sockbuf *data)
{
    const tcp_header *p_hdr = (tcp_header *)data->header(sockbuf_header_t::tcp);
    if (p_hdr->flags & TCP_FLAG_ACK) {
        handle_segment(data);
    } else {
        send_packet(TCP_FLAG_RST);
    }
}

kerror_t tcp_socket::send_syn_ack(const sockbuf *reply_to, uint32_t isn)
{
    const tcp_header *p_in_hdr = (tcp_header *)reply_to->header(sockbuf_header_t::tcp);
    const ip_hdr *p_ip_hdr = (ip_hdr *)reply_to->header(sockbuf_header_t::ip);

    sockbuf *reply = new sockbuf(tcp_layer_->headers_size(), nullptr, 0);
    tcp_header *p_tcp_hdr = (tcp_header *)reply->add_header(sizeof(tcp_header), sockbuf_header_t::tcp);
    p_tcp_hdr->source_port = p_in_hdr->dest_port;
    p_tcp_hdr->dest_port = p_in_hdr->source_port;
    p_tcp_hdr->seq = htonl(isn);
    p_tcp_hdr->ack = htonl(ntohl(p_in_hdr->seq) + reply_to->payload_size() + 1);
    p_tcp_hdr->header_len = (sizeof(tcp_header) / sizeof(uint32_t)) << 4;
    p_tcp_hdr->flags = TCP_FLAG_SYN | TCP_FLAG_ACK;
    p_tcp_hdr->window_size = htons(TCP_INITIAL_WINDOW_SIZE);
    p_tcp_hdr->csum = 0;
    p_tcp_hdr->urp = 0;

    return tcp_layer_->send(ntohl(p_ip_hdr->saddr), reply);
}

kerror_t tcp_socket::send_packet(uint8_t flags)
{
    sockbuf *reply = new sockbuf(tcp_layer_->headers_size(), nullptr, 0);
    tcp_header *p_tcp_hdr = (tcp_header *)reply->add_header(sizeof(tcp_header), sockbuf_header_t::tcp);
    p_tcp_hdr->source_port = htons(port_);
    p_tcp_hdr->dest_port = htons(get_remote_port());
    p_tcp_hdr->seq = htonl(seq_);
    p_tcp_hdr->ack = htonl(ack_);
    p_tcp_hdr->header_len = (sizeof(tcp_header) / sizeof(uint32_t)) << 4;
    p_tcp_hdr->flags = flags;
    p_tcp_hdr->window_size = htons(recv_window_size_ - recv_window_used_);
    p_tcp_hdr->csum = 0;
    p_tcp_hdr->urp = 0;

    return tcp_layer_->send(get_remote_addr(), reply);
}

uint32_t tcp_socket::generate_isn()
{
    return arch_tsc() % UINT32_MAX;
}

} // otrix::net
