#pragma once

#include "net/socket.hpp"
#include "common/hash_map.hpp"

namespace otrix::net
{

class tcp
{
public:
    tcp(ipv4 *ip_layer);

    // Create TCP socket
    socket *create_socket();

    uint16_t tcp_checksum(const sockbuf *data, ipv4_t dest_address_network_order);

private:

    void process_packet(sockbuf *data);

    void send_rst_reply(sockbuf *data);

    struct socket_id
    {
        friend bool operator==(const socket_id& l, const socket_id& r)
        {
            return l.host_port == r.host_port &&
                l.remote_port == r.remote_port &&
                l.remote_ip == r.remote_ip;
        }
        uint16_t host_port;
        uint16_t remote_port;
        uint32_t remote_ip;
    };

    class tcp_socket: public socket {
    public:
        tcp_socket(tcp *tcp_layer);

        size_t send(const void *data, size_t data_size) override;
        size_t recv(void *data, size_t data_size) override;

        struct node_t {
            node_t(tcp_socket *sock): p_socket(sock)
            {}

            hash_map_node<socket_id> hm_node;
            tcp_socket *p_socket;
        };

        static_assert(std::is_standard_layout<node_t>::value, "node_t should have standard layout");

        node_t *node()
        {
            return &node_;
        }

        void process_packet(sockbuf *data)
        {
            (void)data;
        }

    private:
        node_t node_;
    };

    ipv4 *ip_layer_;

    static constexpr auto TCP_LISTEN_TABLE_SIZE = 256;
    // uint16_t -> tcp_socket*
    hash_map<socket_id> listen_sockets_; /**< Map of listening sockets identified by host port only **/

    static size_t socket_id_hash_func(const tcp::socket_id &key);

    static constexpr auto TCP_CONNECTED_TABLE_SIZE = 256;
    // connected_socket_id -> tcp_socket*
    hash_map<socket_id> connected_sockets_; /**< Rest of the sockets identified by more fields **/
};

} // namespace otrix::net
