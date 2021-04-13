#pragma once

#include <cstddef>
#include <cstdint>
#include "common/error.h"

#include "net/ipv4.hpp"

namespace otrix::net
{

enum address_family
{
    af_inet
};

enum socket_type
{
    sock_stream,
};

class socket
{
public:
    virtual ~socket() {}

    virtual size_t send(const void *data, size_t data_size) = 0;
    virtual size_t recv(void *data, size_t data_size) = 0;
    virtual kerror_t connect(ipv4_t remote_addr, ipv4_t remote_port) = 0;
    virtual kerror_t bind(uint16_t port) = 0;
    virtual kerror_t listen(size_t backlog_size) = 0;
    virtual socket *accept(uint64_t timeout_ms = -1) = 0;
    virtual kerror_t shutdown(bool read, bool write) = 0;

    ipv4_t get_remote_addr() const
    {
        return remote_addr_;
    }

    uint16_t get_remote_port() const
    {
        return remote_port_;
    }

protected:
    ipv4_t remote_addr_;
    uint16_t remote_port_;
};

} // namespace otrix::net
