#pragma once

#include <cstddef>
#include <type_traits>

#include "common/error.h"
#include "common/list.h"
#include "arch/context.h"

#define KTHREAD_NODE_PTR(list_ptr) container_of(list_ptr, kthread::node_t, list_node)
#define KTHREAD_PTR(list_ptr) KTHREAD_NODE_PTR(list_ptr)->p_thread

#define KTHREAD_DEFAULT_PRIORITY 0

#define KTHREAD_BLOCK_INF (-1)

namespace otrix
{

// TODO(RostakaGmfun): Add more type safety
using kthread_entry = void(*)();

enum kthread_state {
    KTHREAD_STATE_ZOMBIE,  /**< Not present int any of the thread queues **/
    KTHREAD_STATE_ACTIVE,  /**< Current thread **/
    KTHREAD_STATE_RUNNABLE,/**< In the runnable queue **/
    KTHREAD_STATE_BLOCKED, /**< Blocked **/
};

/**
 * Represents a single-core kernel cooperating threads (fibers).
 */
class kthread
{
public:
    kthread(uint64_t *stack, size_t stack_size, kthread_entry entry, int priority = KTHREAD_DEFAULT_PRIORITY);
    kthread(const kthread &other) = delete;
    kthread(kthread &&other);

    kthread &operator=(const kthread &other) = delete;
    kthread &operator=(kthread &&other);

    ~kthread();

    int get_id() const { return id_; }

    // standard-layout type to contain the intrusive list node
    struct node_t
    {
        intrusive_list list_node;
        kthread *p_thread;
        uint64_t tsc_deadline;
        kthread_state state;
    };

    static_assert(std::is_standard_layout<node_t>::value, "node_t should have standard layout");

    node_t *node()
    {
        return &node_;
    }

    int priority() const {
        return priority_;
    }

    arch_context *context() {
        return &context_;
    }

private:
    arch_context context_;
    int id_;
    uint64_t *stack_;
    size_t stack_size_;
    kthread_entry entry_;
    node_t node_;
    int priority_;
};

class scheduler
{
public:
    scheduler(const scheduler &other) = delete;
    scheduler(scheduler &&other) = delete;

    scheduler &operator=(const scheduler &other) = delete;
    scheduler &operator=(scheduler &&other) = delete;

    static scheduler &get();

    static void timer_irq_handler(void *p_ctx);

    /**
     * Add a thread to the scheduling list.
     */
    kerror_t add_thread(kthread *thread);

    kerror_t remove_thread(kthread *thread);

    /**
     * Move thread to blocked queue (inifinte or with timeout).
     */
    kerror_t block_thread(kthread *thread, uint64_t block_time_ms);

    /**
     * Move thread from blocked queue to run queue.
     */
    kerror_t unblock_thread(kthread *thread);

    /**
     * Retrieve the currently running thread.
     */
    kthread *get_current_thread() {
        return KTHREAD_PTR(current_thread_);
    }

    /**
     * Schedule the next thread to run.
     */
    kerror_t schedule();

private:
    scheduler();

    void handle_timer_irq();

    static constexpr auto NUM_PRIORITIES = 10;
    intrusive_list *runnable_queues_[NUM_PRIORITIES];
    intrusive_list *blocked_queues_[NUM_PRIORITIES];
    intrusive_list *current_thread_;
    bool need_resched_;
    uint64_t nearest_tsc_deadline_;
};

} // namespace otrix
