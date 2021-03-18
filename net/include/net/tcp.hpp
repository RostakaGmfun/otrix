#pragma once

#include "common/hash_map.hpp"
#include "common/error.h"
#include "net/ipv4.hpp"
#include <functional>

namespace otrix::net
{

enum tcp_flags {
    TCP_FLAG_CONG_WND_REDUCED = 1 << 7,
    TCP_FLAG_ECN_ECHO         = 1 << 6,
    TCP_FLAG_URG              = 1 << 5,
    TCP_FLAG_ACK              = 1 << 4,
    TCP_FLAG_PSH              = 1 << 3,
    TCP_FLAG_RST              = 1 << 2,
    TCP_FLAG_SYN              = 1 << 1,
    TCP_FLAG_FIN              = 1 << 0,
};

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


class socket;
class tcp_socket;
class sockbuf;
class ipv4;

class tcp
{
public:
    tcp(ipv4 *ip_layer);

    // Create TCP socket
    socket *create_socket();

    uint16_t tcp_checksum(const sockbuf *data, ipv4_t dest_address_network_order);

    struct socket_id
    {
        bool operator==(const socket_id& r)
        {
            return host_port == r.host_port &&
                remote_port == r.remote_port &&
                remote_ip == r.remote_ip;
        }

        static size_t hash_func(const tcp::socket_id &key)
        {
            size_t ret = std::hash<uint16_t>{}(key.host_port);
            ret ^= std::hash<uint16_t>{}(key.remote_port) + 0x9e3779b9 + (ret<<6) + (ret>>2);
            ret ^= std::hash<uint32_t>{}(key.remote_ip) + 0x9e3779b9 + (ret<<6) + (ret>>2);
            return ret;
        }

        uint16_t host_port;
        uint16_t remote_port;
        uint32_t remote_ip;
    };

    // TODO: Make these a private API only available to tcp_socket class
    kerror_t bind_socket(tcp_socket *sock, uint16_t port);
    kerror_t unbind_socket(tcp_socket *sock);
    kerror_t send_reply(const sockbuf *reply_to, uint8_t flags);
    kerror_t add_connected_socket(tcp_socket *sock, socket_id id);
    kerror_t remove_connected_socket(tcp_socket *sock);

private:

    void process_packet(sockbuf *data);

    ipv4 *ip_layer_;

    static constexpr auto TCP_LISTEN_TABLE_SIZE = 257;
    // uint16_t -> tcp_socket*
    hash_map<socket_id> listen_sockets_; /**< Map of listening sockets identified by host port only **/

    static constexpr auto TCP_CONNECTED_TABLE_SIZE = 257;
    // connected_socket_id -> tcp_socket*
    hash_map<socket_id> connected_sockets_; /**< Rest of the sockets identified by more fields **/
};

} // namespace otrix::net
