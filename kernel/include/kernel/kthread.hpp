#ifndef OTRIX_KTHREAD_HPP
#define OTRIX_KTHREAD_HPP

#include <cstddef>
#include <type_traits>
#include <array>
#include <experimental/optional>

#include "common/error.hpp"
#include "arch/context.h"

namespace otrix
{

template <typename T>
using optional_t = std::experimental::optional<T>;

// TODO(RostakaGmfun): Add more type safety
using kthread_entry = void(*)(void *);

/**
 * Represents a single-core kernel cooperating threads (fibers).
 */
class kthread
{
public:
    kthread();
    kthread(uint64_t *stack, size_t stack_size, kthread_entry entry);
    kthread(const kthread &other) = delete;
    kthread(kthread &&other);

    kthread &operator=(const kthread &other) = delete;
    kthread &operator=(kthread &&other);

    ~kthread();

    uint32_t get_id() const { return id_; }

    void run()
    {
        arch_context_restore(&context_);
    }

    /**
     * Suspend the thread allowing other threads to run.
     */
    error yield();

private:
    arch_context context_;
    uint32_t id_;
    uint64_t *stack_;
    size_t stack_size_;
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
    static error add_thread(kthread &thread);

    static error remove_thread(uint32_t thread_id);

    static optional_t<kthread *> get_thread_by_id(const uint32_t thread_id);

    /**
     * Retrieve the currently running thread.
     */
    static kthread &get_current_thread();

    /**
     * Schedule the next thread to run.
     */
    static error schedule();

private:
    // TODO(RostakaGmfun): Add compilation option for array size
    static constexpr auto thread_queue_size = 16;
    static std::array<kthread, thread_queue_size> threads_;
    static size_t current_thread_;
    static kthread idle_thread_;
    static std::size_t num_threads_;
};

} // namespace otrix

#endif // OTRIX_KTHREAD_HPP
