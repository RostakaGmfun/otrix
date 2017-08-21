#ifndef OTRIX_KTHREAD_HPP
#define OTRIX_KTHREAD_HPP

#include "common/error.hpp"
#include "arch/common.hpp"
#include "arch/context.h"

namespace otrix
{

/**
 * Represents a single-core kernel cooperating threads (fibers).
 */
class kthread
{
public:
    kthread() = default;
    kthread(const kthread &other) = default;
    kthread(kthread &&other) = default;

    kthread &operator=(const kthread &other) = default;
    kthread &operator=(kthread &other) = default;

    ~kthread() = default;

    uint32_t get_id() const { return id_; }

    /**
     * Suspend the thread allowing other threads to run.
     */
    error_code yield();

private:
    arch_context context;
    uint32_t id;
    std::aligned_storage<otrix::arch::word_type, otrix::arch::stack_align> stack;
};

class kthread_scheduler
{
public:
    kthread_scheduler() = delete;
    kthread_scheduler(const kthread_scheduler &other) = delete;
    kthread_scheduler(kthread_scheduler &&other) = delete;

    kthread_scheduler &operator=(const kthread_scheduler &other) = delete;
    kthread_scheduler &operator=(kthread_scheduler &&other) = delete;

    /**
     * Add a thread to the scheduling queue.
     */
    static error_code add_thread(kthread &&thread);

    /**
     * Retrieve the currently running thread.
     */
    static kthread &current_thread();

    /**
     * Schedule the next thread to run.
     */
    static error_code schedule();
};

} // namespace otrix

#endif // OTRIX_KTHREAD_HPP
