#pragma once

#include <cstddef>
#include <cstring>
#include <cstdint>
#include <cstddef>
#include "kernel/alloc.hpp"
#include "common/list.h"

namespace otrix::net
{

enum class sockbuf_header_t
{
    virtio,
    ethernet,
    ip,
    icmp,
    udp,
    tcp,

    max,
};

class sockbuf
{
public:

    // Buffer free function for zero-copy socket buffers
    typedef void (*free_func_t)(void *buf, size_t size, void *ctx);

    sockbuf(size_t headers_size, const uint8_t *payload, size_t payload_size):
        buffer_size_(headers_size + payload_size), payload_size_(payload_size), free_func_(nullptr), node_(this)
    {
        start_ = (uint8_t *)otrix::alloc(headers_size + payload_size);
        if (nullptr != start_) {
            head_ = (uint8_t *)start_ + headers_size;
            payload_ = head_;
            if (nullptr != payload) {
                memcpy(payload_, payload, payload_size);
            }
            for (auto &hdr : headers_) {
                hdr = nullptr;
            }
        }
    }

    // Zero-copy interface
    sockbuf(uint8_t *data, size_t data_size, free_func_t free_func, void *free_func_ctx):
        start_(data), buffer_size_(data_size), payload_(data), head_(data), payload_size_(data_size), free_func_(free_func),
        free_func_ctx_(free_func_ctx), node_(this)
    {
        for (auto &hdr : headers_) {
            hdr = nullptr;
        }
    }

    sockbuf& operator=(const sockbuf &other) = default;

    sockbuf(sockbuf &&other): node_(this)
    {
        *this = other;
        node_.p_skb = this;
        intrusive_list_init(&node_.list_node);
        other.start_ = nullptr;
        memset(other.headers_, 0, sizeof(other.headers_));
        other.buffer_size_ = 0;
        other.payload_ = nullptr;
        other.head_ = nullptr;
        other.payload_size_ = 0;
        other.free_func_ = nullptr;
        other.free_func_ctx_ = nullptr;
    }

    ~sockbuf()
    {
        if (nullptr != free_func_) {
            free_func_(start_, buffer_size_, free_func_ctx_);
        } else if (nullptr != start_) {
            otrix::free(start_);
        }
    }

    const void *data() const
    {
        return nullptr == start_ ? start_ : head_;
    }

    void *data()
    {
        return nullptr == start_ ? start_ : head_;
    }

    size_t size() const
    {
        return (payload_ - head_) + payload_size_;
    }

    uint8_t *add_header(size_t header_size, sockbuf_header_t type)
    {
        if ((size_t)(head_ - start_) < header_size) {
            return nullptr;
        }

        if (nullptr != headers_[(int)type]) {
            return headers_[(int)type];
        }

        head_ -= header_size;
        headers_[(int)type] = head_;
        return head_;
    }

    void add_parsed_header(size_t header_size, sockbuf_header_t type)
    {
        if (nullptr != headers_[(int)type]) {
            return;
        }
        if (header_size > payload_size_) {
            return;
        }
        headers_[(int)type] = head_;
        head_ += header_size;
        payload_size_ -= header_size;
        payload_ += header_size;
    }

    void set_payload_size(size_t new_size)
    {
        payload_size_ = new_size;
    }

    const uint8_t *header(sockbuf_header_t type) const
    {
        return headers_[(int)type];
    }

    uint8_t *header(sockbuf_header_t type)
    {
        return headers_[(int)type];
    }

    const uint8_t *payload() const
    {
        return payload_;
    }

    uint8_t *payload()
    {
        return payload_;
    }

    size_t payload_size() const
    {
        return payload_size_;
    }

    struct node_t {
        node_t(sockbuf *skb): p_skb(skb)
        {}
        intrusive_list list_node;
        sockbuf *p_skb;
    };

    node_t *node() {
        return &node_;
    }

    const node_t *node() const {
        return &node_;
    }

private:
    uint8_t *start_;
    size_t buffer_size_;
    uint8_t *headers_[(int)sockbuf_header_t::max];
    uint8_t *payload_;
    uint8_t *head_;
    size_t payload_size_;
    free_func_t free_func_;
    void *free_func_ctx_;
    node_t node_;
};

} //namespace otrix::net
