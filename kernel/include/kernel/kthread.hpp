#pragma once

#include <cstddef>
#include <type_traits>

#include "common/error.h"
#include "common/list.h"
#include "arch/context.h"

#define KTHREAD_PTR(list_ptr) container_of(list_ptr, kthread::node_t, list_node)->p_thread

#define KTHREAD_DEFAULT_PRIORITY 0

namespace otrix
{

// TODO(RostakaGmfun): Add more type safety
using kthread_entry = void(*)();

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

    /**
     * Suspend the thread allowing other threads to run.
     */
    kerror_t yield();

    // standard-layout type to contain the intrusive list node
    struct node_t
    {
        intrusive_list list_node;
        kthread *p_thread;
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

    /**
     * Add a thread to the scheduling list.
     */
    kerror_t add_thread(kthread *thread);

    kerror_t remove_thread(kthread *thread);

    kthread *get_thread_by_id(const uint32_t thread_id);

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

    static constexpr auto NUM_PRIORITIES = 10;
    intrusive_list *run_queues_[NUM_PRIORITIES];
    intrusive_list *current_thread_;
};

} // namespace otrix
