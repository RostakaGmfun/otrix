#pragma once

#include "kernel/semaphore.hpp"
#include "net/ipv4.hpp"

namespace otrix::net
{

// forward declaration
class sockbuf;

class icmp
{
public:
    icmp(ipv4 *ip);

    bool ping(ipv4_t addr, uint64_t timeout);

private:
    void process_packet(sockbuf *data);

    void process_echo_request(sockbuf *data);
    void process_echo_reply(sockbuf *data);

    ipv4 *ip_;

    semaphore ping_req_semaphore_;
    bool ping_req_pending_;
    uint16_t ping_req_id_;
};

} // namespace otrix::net
