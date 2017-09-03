#include "kernel/kthread.hpp"
#include <algorithm>

namespace otrix
{

std::array<kthread, kthread_scheduler::thread_queue_size> kthread_scheduler::threads_;
size_t kthread_scheduler::current_thread_;
kthread kthread_scheduler::idle_thread_;
std::size_t kthread_scheduler::num_threads_;

kthread::kthread(): stack_(nullptr), stack_size_(0), entry_(nullptr)
{}

kthread::kthread(uint64_t *stack, size_t stack_size, kthread_entry entry):
    entry_(entry), stack_(stack), stack_size_(stack_size)
{
    static int largest_id = 1;
    id_ = largest_id++;
    arch_context_setup(&context_, &stack_,
            stack_size, entry_, nullptr);
}

kthread::kthread(kthread &&other)
{
    *this = std::move(other);
}

kthread &kthread::operator=(kthread &&other)
{
    id_ = other.id_;
    other.id_ = 0;
    context_ = other.context_;
    stack_ = other.stack_;
    other.stack_ = nullptr;
    stack_size_ = other.stack_size_;
    other.stack_size_ = 0;
    entry_ = other.entry_;
    other.entry_ = nullptr;
    return *this;
}

kthread::~kthread()
{
    if (id_ > 0) {
        kthread_scheduler::remove_thread(get_id());
    }
}

error kthread::yield()
{
    arch_context_save(&context_);
    kthread_scheduler::schedule();
}

error kthread_scheduler::add_thread(kthread &&thread)
{
    if (num_threads_ > thread_queue_size) {
        return error::nomem;
    }

    threads_[num_threads_++] = std::move(thread);

    return error::ok;
}

error kthread_scheduler::remove_thread(const uint32_t thread_id)
{
    auto t = get_thread_by_id(thread_id);
    if (t != std::experimental::nullopt) {
        auto &last = threads_[num_threads_--];
        std::swap(*(t.value()), last);
        return error::ok;
    }

    return error::inval;
}

optional_t<kthread *> kthread_scheduler::get_thread_by_id(const uint32_t thread_id)
{
    auto it = std::find_if(threads_.begin(), threads_.end(),
        [thread_id] (const auto &t)
        {
            return t.get_id() == thread_id;
        });
    if (it != threads_.end()) {
        return &(*it);
    }

    return {};
}

const kthread &kthread_scheduler::get_current_thread()
{
    return threads_[current_thread_];
}


error kthread_scheduler::schedule()
{
    current_thread_++;
    current_thread_ %= num_threads_;
    threads_[current_thread_].run();
}

}
