#ifndef OTRIX_KTHREAD_HPP
#define OTRIX_KTHREAD_HPP

#include <cstddef>
#include <type_traits>

#include "common/error.hpp"
#include "arch/common.hpp"
#include "arch/context.h"

namespace otrix
{

// TODO(RostakaGmfun): Add more type safety
using kthread_entry = (void)(*)(void *);

/**
 * Represents a single-core kernel cooperating threads (fibers).
 */
class kthread
{
public:
    kthread(kthread_entry &&entry): entry_(entry)
    {
        arch_context_setup(&context_, &stack_,
                &stack_, entry_, nullptr);
    }

    kthread(const kthread &other) = default;
    kthread(kthread &&other) = default;

    kthread &operator=(const kthread &other) = default;
    kthread &operator=(kthread &other) = default;

    ~kthread()
    {
        kthread_scheduler::remove_thread(get_id());
    }

    uint32_t get_id() const { return id_; }

    void run()
    {
        arch_context_restore(&context_);
    }

    /**
     * Suspend the thread allowing other threads to run.
     */
    error_code yield()
    {
        arch_context_save(&context_);
    }

private:
    arch_context context_;
    uint32_t id_;
    std::aligned_storage<512, 16> stack_;
    kthread_entry entry_;
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
     * Add a thread to the scheduling list.
     */
    static error_code add_thread(kthread &&thread);

    static error_code remove_thread(uint32_t thread_id);

    /**
     * Retrieve the currently running thread.
     */
    static kthread &current_thread();

    /**
     * Schedule the next thread to run.
     */
    static error_code schedule();

private:
    // TODO(RostakaGmfun): Add compilation option for array size
    std::array<kthread, 16> threads_;
    kthread idle_thread_;
    std::size_t num_threads;
};

} // namespace otrix

#endif // OTRIX_KTHREAD_HPP
