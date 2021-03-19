#pragma once

#include <cstddef>
#include <type_traits>

#include "common/error.h"
#include "common/list.h"
#include "arch/context.h"

#define KTHREAD_NODE_PTR(list_ptr) container_of(list_ptr, kthread::node_t, list_node)
#define KTHREAD_PTR(list_ptr) KTHREAD_NODE_PTR(list_ptr)->p_thread

#define KTHREAD_DEFAULT_PRIORITY 0

#define KTHREAD_TIMEOUT_INF (-1)

namespace otrix
{

using kthread_entry = void(*)(void *);

enum kthread_state {
    KTHREAD_STATE_ZOMBIE,  /**< Not present in any of the thread queues **/
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
    kthread(size_t stack_size, kthread_entry entry, const char *name, int priority = KTHREAD_DEFAULT_PRIORITY, void *ctx = nullptr);
    kthread(const char *name, int priority = KTHREAD_DEFAULT_PRIORITY);
    kthread(const kthread &other) = delete;
    kthread(kthread &&other) = delete;

    kthread &operator=(const kthread &other) = delete;
    kthread &operator=(kthread &&other) = delete;

    ~kthread();

    // standard-layout type to contain the intrusive list node
    struct node_t
    {
        node_t(kthread *thread): p_thread(thread), tsc_deadline(0), state(KTHREAD_STATE_ZOMBIE)
        {
            intrusive_list_init(&list_node);
        }
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

    const char *name() const {
        return name_;
    }

private:
    arch_context context_;
    uint64_t *stack_;
    size_t stack_size_;
    kthread_entry entry_;
    node_t node_;
    int priority_;
    const char *name_;
};

class scheduler
{
public:
    scheduler(const scheduler &other) = delete;
    scheduler(scheduler &&other) = delete;

    scheduler &operator=(const scheduler &other) = delete;
    scheduler &operator=(scheduler &&other) = delete;

    static scheduler &get();

    static void handle_timer_irq(void *p_ctx);

    /**
     * Add a thread to the scheduling list.
     */
    kerror_t add_thread(kthread *thread);

    kerror_t remove_thread(kthread *thread);

    /**
     * Move current thread to blocked queue (inifinte or with timeout).
     */
    kerror_t sleep(uint64_t sleep_time_ms);

    kerror_t sleep_until(uint64_t tsc_deadline);

    /**
     * Move thread from blocked queue to run queue.
     */
    kerror_t wake(kthread *thread);

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

    /**
     * Disable task switching.
     */
    void preempt_disable();

    /**
     * Enable task switching.
     */
    void preempt_enable(bool reschedule = true);

private:
    scheduler();

    void setup_timer();

    void handle_timer_irq();

    static constexpr auto NUM_PRIORITIES = 10;
    intrusive_list *runnable_queues_[NUM_PRIORITIES];
    intrusive_list *blocked_queues_[NUM_PRIORITIES];
    intrusive_list *current_thread_;
    bool need_resched_;
    uint64_t nearest_tsc_deadline_;

    kthread idle_thread_;
    int preempt_disable_;
};

} // namespace otrix
