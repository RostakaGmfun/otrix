#pragma once

#include "common/list.h"
#include "kernel/mutex.hpp"

namespace otrix
{

class kthread;

using timer_cb_t = void (*)(void *ctx, void *shared_ctx);
using timer_task_t = intrusive_list;

class timer_service
{
public:
    timer_service(int priority, void *shared_ctx = nullptr);

    ~timer_service();

    timer_service(const timer_service &other) = delete;
    timer_service(kthread &&other) = delete;

    timer_service &operator=(const kthread &other) = delete;
    timer_service &operator=(kthread &&other) = delete;

    /**
     * Schedule call to cb with context in ctx after timeout_ms milliseconds elapsed.
     *
     * @return Handle to use in remove_task().
     */
    timer_task_t *add_task(timer_cb_t cb, void *ctx, uint64_t timeout_ms);

    /**
     * Remove task from the pending queue.
     */
    void remove_task(timer_task_t *p_task);

private:

    // Thread entry
    void run();

    struct task_desc_t
    {
        intrusive_list list_node;
        timer_cb_t p_cb;
        void *p_ctx;
        uint64_t deadline_tsc;
    };

    mutex mutex_;
    intrusive_list *pending_tasks_; // task_desc_t
    intrusive_list *task_pool_; // free descriptors pool to allocate from
    task_desc_t *task_pool_ptr_;
    void *shared_ctx_;
    kthread *service_thread_;
};

} // namespace otrix
