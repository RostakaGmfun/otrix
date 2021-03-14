#pragma once

namespace otrix::net
{

// forward declaration
class ipv4;
class sockbuf;

class icmp
{
public:
    icmp(ipv4 *ip);

private:
    void process_packet(sockbuf *data);

    void process_echo_request(sockbuf *data);

    ipv4 *ip_;
};

} // namespace otrix::net
