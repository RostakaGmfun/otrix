#include "kernel/kthread.hpp"
#include <algorithm>
#include "arch/asm.h"
#include "otrix/immediate_console.hpp"

namespace otrix
{

kthread::kthread(uint64_t *stack, size_t stack_size, kthread_entry entry, int priority):
    stack_(stack), stack_size_(stack_size), entry_(entry), priority_(priority)
{
    static int largest_id = 1;
    id_ = largest_id++;
    arch_context_setup(&context_, stack_,
            stack_size, entry_);
    intrusive_list_init(&node_.list_node);
    node_.p_thread = this;
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
        scheduler::get().remove_thread(this);
    }
}

scheduler::scheduler(): current_thread_(nullptr)
{
    for (int i = 0; i < NUM_PRIORITIES; i++) {
        run_queues_[i] = nullptr;
    }
}

scheduler &scheduler::get() {
    static scheduler instance;
    return instance;
}

kerror_t scheduler::add_thread(kthread *thread)
{
    if (nullptr == thread) {
        return E_INVAL;
    }
    const int prio = thread->priority();
    const auto flags = arch_irq_save();
    if (nullptr == run_queues_[prio]) {
        run_queues_[prio] = &thread->node()->list_node;
    } else {
        intrusive_list_push_back(run_queues_[prio], &thread->node()->list_node);
    }
    arch_irq_restore(flags);
    return E_OK;
}

kerror_t scheduler::remove_thread(kthread *thread)
{
    if (nullptr == thread) {
        return E_INVAL;
    }
    const int prio = thread->priority();
    const auto flags = arch_irq_save();
    if (run_queues_[prio] == &thread->node()->list_node) {
        run_queues_[prio] = nullptr;
    } else {
        intrusive_list_unlink_node(&thread->node()->list_node);
    }
    arch_irq_restore(flags);
    return E_OK;
}

kerror_t scheduler::schedule()
{
    intrusive_list *prev_thread = current_thread_;
    auto flags = arch_irq_save();
    // Advance thread pointer in the current queue
    if (nullptr != current_thread_) {
        current_thread_ = current_thread_->next;
    }

    int prio = NUM_PRIORITIES - 1;
    // If end of queue reached, select queue with lower priority
    while (nullptr == current_thread_ && prio >= 0) {
        current_thread_ = run_queues_[prio];
        prio--;
    }

    if (nullptr != current_thread_) {
        if (nullptr == prev_thread) {
            arch_context *ctx = KTHREAD_PTR(current_thread_)->context();
            arch_context_restore(ctx);
        } else {
            arch_context_switch(KTHREAD_PTR(prev_thread)->context(),
                    KTHREAD_PTR(current_thread_)->context());
        }
    }

    arch_irq_restore(flags);

    return E_OK;
}

} // namespace otrix
