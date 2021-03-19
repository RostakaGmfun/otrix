#pragma once

#include "kernel/waitq.hpp"

namespace otrix
{

class kthread;

class mutex
{
public:
    mutex();
    ~mutex();

    mutex(const mutex &other) = delete;
    mutex(mutex &&other) = delete;

    mutex &operator=(const mutex &other) = delete;
    mutex &operator=(mutex &&other) = delete;

    bool lock(uint64_t timeout_ms = -1);
    void unlock();

private:
    waitq waiting_queue_;
    kthread *owner_;
};

} // namespace otrix
