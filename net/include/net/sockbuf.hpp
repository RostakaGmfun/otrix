#pragma once

#include <cstddef>
#include <cstring>
#include "kernel/alloc.hpp"

namespace otrix::net
{

enum class sockbuf_header_t
{
    virtio,
    ethernet,
    ip,
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
        buffer_size_(headers_size + payload_size), payload_size_(payload_size), free_func_(nullptr)
    {
        start_ = (uint8_t *)otrix::alloc(headers_size + payload_size);
        if (nullptr != start_) {
            head_ = (uint8_t *)start_ + headers_size;
            payload_ = head_;
            memcpy(payload_, payload, payload_size);
            for (auto &hdr : headers_) {
                hdr = nullptr;
            }
        }
    }

    // Zero-copy interface
    sockbuf(uint8_t *data, size_t data_size, free_func_t free_func, void *free_func_ctx):
        start_(data), buffer_size_(data_size), payload_(data), head_(data), payload_size_(data_size), free_func_(free_func),
        free_func_ctx_(free_func_ctx)
    {
        for (auto &hdr : headers_) {
            hdr = nullptr;
        }
    }

    ~sockbuf()
    {
        if (nullptr != free_func_) {
            free_func_(start_, buffer_size_, free_func_ctx_);
        } else if (nullptr != start_) {
            otrix::free(start_);
        }
    }

    sockbuf(const sockbuf &other);
    sockbuf& operator=(const sockbuf &other);

    sockbuf(sockbuf &&other);
    sockbuf& operator=(sockbuf &&other);

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
        return buffer_size_;
    }

    uint8_t *add_header(size_t header_size, sockbuf_header_t type)
    {
        if ((size_t)(head_ - start_) < header_size || nullptr != headers_[(int)type]) {
            return nullptr;
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

    const uint8_t *header(sockbuf_header_t type) const
    {
        return headers_[(int)type];
    }

    const void *payload() const
    {
        return payload_;
    }

    size_t payload_size() const
    {
        return payload_size_;
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
};

} //namespace otrix::net
