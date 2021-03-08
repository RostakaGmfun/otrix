#pragma once

#include "kernel/waitq.hpp"

namespace otrix
{

class semaphore
{
public:
    semaphore(int max_count = 1, int initial_count = 0);
    ~semaphore();

    semaphore(const semaphore &other) = delete;
    semaphore(semaphore &&other) = delete;

    semaphore &operator=(const semaphore &other) = delete;
    semaphore &operator=(semaphore &&other) = delete;

    bool take(uint64_t timeout_ms = -1);
    void give();

    int count() const
    {
        return count_;
    }

private:
    waitq waiting_queue_;
    int count_;
    int max_count_;
};

} // namespace otrix
