#pragma once

#include "kernel/semaphore.hpp"

namespace otrix
{

class msgq
{
public:
    msgq(size_t size, size_t msg_size);
    ~msgq();

    msgq(const msgq &other) = delete;
    msgq(msgq &&other) = delete;

    msgq &operator=(const msgq &other) = delete;
    msgq &operator=(msgq &&other) = delete;

    template<typename T>
    bool read(T *msg, uint64_t timeout_ms)
    {
        return read(reinterpret_cast<void *>(msg), timeout_ms);
    }

    template<typename T>
    bool write(const T *msg)
    {
        return write(reinterpret_cast<const void *>(msg));
    }

    bool read(void *msg, uint64_t timeout_ms);

    bool write(const void *msg);

    bool full() const {
        return sem_.count() == (int)size_;
    }

private:
    uint8_t *storage_;
    size_t read_p_;
    size_t write_p_;
    size_t size_;
    size_t msg_size_;
    semaphore sem_;
};

} // namespace otrix
