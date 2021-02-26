#pragma once

#include "common/list.h"
#include "kernel/kthread.hpp"

namespace otrix
{

class waitq
{
public:
    waitq();
    waitq(const waitq &other) = delete;
    waitq &operator=(const waitq &other) = delete;
    ~waitq();

    bool wait(uint64_t timeout_ms = KTHREAD_BLOCK_INF);
    void notify_one();
    void notify_all();

private:

    static void unblock_handler(void *ctx);

    // Context for blocked thread
    struct waitq_item
    {
        intrusive_list list_node;
        bool wakeup_successful;
        kthread *thread;
        waitq *self;
    };

    intrusive_list *wq_;
};

} // namespace otrix
