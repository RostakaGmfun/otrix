#pragma once

#include "net/socket.hpp"
#include "net/tcp.hpp"
#include "common/hash_map.hpp"

namespace otrix
{
class msgq;
}

namespace otrix::net
{

class sockbuf;
class tcp;

enum tcp_state
{
    TCP_STATE_CLOSED,
    TCP_STATE_LISTEN,
    TCP_STATE_SYN_SENT,
    TCP_STATE_SYN_RECEIVED,
    TCP_STATE_ESTABLISHED,
    TCP_STATE_FIN_WAIT1,
    TCP_STATE_FIN_WAIT2,
    TCP_STATE_CLOSE_WAIT,
    TCP_STATE_CLOSING,
    TCP_STATE_LAST_ACK,
    TCP_STATE_TIME_WAIT,
};

class tcp_socket: public socket {
public:
    tcp_socket(tcp *tcp_layer);
    ~tcp_socket() override;

    size_t send(const void *data, size_t data_size) override;
    size_t recv(void *data, size_t data_size) override;

    kerror_t connect(ipv4_t remote_addr, ipv4_t remote_port) override;
    kerror_t bind(uint16_t port) override;
    kerror_t listen(size_t backlog_size) override;
    socket *accept(uint64_t timeout_ms = -1) override;
    kerror_t shutdown() override;

    struct node_t {
        node_t(tcp_socket *sock): p_socket(sock)
        {}

        hash_map_node<tcp::socket_id> hm_node;
        tcp_socket *p_socket;
    };

    static_assert(std::is_standard_layout<node_t>::value, "node_t should have standard layout");

    node_t *node()
    {
        return &node_;
    }

    void process_packet(sockbuf *data);

    tcp_state get_state() const
    {
        return state_;
    }

private:

    kerror_t accept_connection(const sockbuf *data);
    kerror_t handle_syn(const sockbuf *data);

    void handle_listen_state(const sockbuf *data);

    static constexpr auto INVALID_PORT = 0xFFFFFFFF;

    node_t node_;
    tcp *tcp_layer_;
    uint32_t port_;
    tcp_state state_;
    msgq *listen_backlog_;

    struct syn_cache_entry
    {
        uint32_t isn; // ACK value sent in SYN+ACK
        uint32_t remote_isn; // SEQ value received in SYN
    };

    using syncache_t = pooled_hash_map<tcp::socket_id, syn_cache_entry>;

    static constexpr auto SYN_CACHE_TABLE_SIZE = 17;
    // socket_id -> syn_cache_entry
    pooled_hash_map<tcp::socket_id, syn_cache_entry> *syn_cache_;
};

} // otrix::net
