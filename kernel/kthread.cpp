#include "kernel/kthread.hpp"
#include <algorithm>
#include "arch/asm.h"
#include "otrix/immediate_console.hpp"
#include "arch/kvmclock.hpp"
#include "arch/lapic.hpp"

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

scheduler::scheduler(): current_thread_(nullptr), need_resched_(false), nearest_tsc_deadline_(-1)
{
    for (int i = 0; i < NUM_PRIORITIES; i++) {
        runnable_queues_[i] = nullptr;
    }
    for (int i = 0; i < NUM_PRIORITIES; i++) {
        blocked_queues_[i] = nullptr;
    }
}

scheduler &scheduler::get() {
    static scheduler instance;
    return instance;
}

void scheduler::timer_irq_handler(void *p_ctx)
{
    (void)p_ctx;
    get().handle_timer_irq();
}

kerror_t scheduler::add_thread(kthread *thread)
{
    if (nullptr == thread) {
        return E_INVAL;
    }
    const int prio = thread->priority();
    const auto flags = arch_irq_save();
    thread->node()->state = KTHREAD_STATE_RUNNABLE;
    runnable_queues_[prio] = intrusive_list_push_back(runnable_queues_[prio], &thread->node()->list_node);
    if (nullptr != current_thread_ && KTHREAD_PTR(current_thread_)->priority() < prio) {
        need_resched_ = true;
    }
    arch_irq_restore(flags);
    return E_OK;
}

kerror_t scheduler::remove_thread(kthread *thread)
{
    if (nullptr == thread) {
        return E_INVAL;
    }
    const auto flags = arch_irq_save();

    const int prio = thread->priority();
    thread->node()->state = KTHREAD_STATE_ZOMBIE;
    runnable_queues_[prio] = intrusive_list_delete(runnable_queues_[prio], &thread->node()->list_node);
    arch_irq_restore(flags);
    return E_OK;
}

kerror_t scheduler::schedule()
{
    intrusive_list *prev_thread = current_thread_;
    auto flags = arch_irq_save();

    int prio = NUM_PRIORITIES - 1;
    // Select highest-priority thread from the run queues
    while (prio >= 0) {
        current_thread_ = runnable_queues_[prio];
        if (nullptr != current_thread_) {
            break;
        }
        prio--;
    }

    KTHREAD_PTR(current_thread_)->node()->state = KTHREAD_STATE_ACTIVE;

    if (nullptr != current_thread_) {
        // Advance queue to the next thread to be picked next time
        runnable_queues_[prio] = runnable_queues_[prio]->next;
        if (nullptr == prev_thread) {
            // First call to schedule: switch to the first thread
            arch_context_restore(KTHREAD_PTR(current_thread_)->context());
        } else {
            arch_context_switch(KTHREAD_PTR(prev_thread)->context(),
                    KTHREAD_PTR(current_thread_)->context());
        }
    }

    arch_irq_restore(flags);

    return E_OK;
}

kerror_t scheduler::block_thread(kthread *thread, uint64_t block_time_ms, void (*handler)(void *), void *p_ctx)
{
    auto flags = arch_irq_save();

    remove_thread(thread);
    intrusive_list *prev_thread = nullptr;
    thread->node()->unblock_handler = handler;
    thread->node()->unblock_handler_ctx = p_ctx;
    thread->node()->state = KTHREAD_STATE_BLOCKED;
    if (static_cast<uint64_t>(-1) == block_time_ms) {
        thread->node()->tsc_deadline = -1;
        if (nullptr != blocked_queues_[thread->priority()]) {
            prev_thread = blocked_queues_[thread->priority()]->prev;
        }
    } else {
        const auto tsc_deadline = arch_tsc() + arch::kvmclock::ns_to_tsc(block_time_ms * 1000 * 1000);
        thread->node()->tsc_deadline = tsc_deadline;
        nearest_tsc_deadline_ = std::min(nearest_tsc_deadline_, tsc_deadline);
        if (nullptr != blocked_queues_[thread->priority()]) {
            do {
                const auto tsc = container_of(prev_thread, kthread::node_t, list_node)->tsc_deadline;
                if (0 == tsc || tsc >= tsc_deadline) {
                    break;
                }
                prev_thread = prev_thread->next;
            } while (prev_thread != blocked_queues_[thread->priority()]);
        }
    }

    if (nullptr == prev_thread) {
        intrusive_list_init(&thread->node()->list_node);
        blocked_queues_[thread->priority()] = &thread->node()->list_node;
    } else {
        intrusive_list_insert(prev_thread, &thread->node()->list_node);
    }

    if (nearest_tsc_deadline_ != static_cast<uint64_t>(-1)) {
        arch::local_apic::start_timer(nearest_tsc_deadline_);
    }

    arch_irq_restore(flags);

    return E_OK;
}

kerror_t scheduler::unblock_thread(kthread *thread)
{
    auto flags = arch_irq_save();

    if (thread->node()->state != KTHREAD_STATE_BLOCKED) {
        arch_irq_restore(flags);
        return E_INVAL;
    }

    blocked_queues_[thread->priority()] = intrusive_list_delete(blocked_queues_[thread->priority()], &thread->node()->list_node);

    add_thread(thread);

    if (nullptr != thread->node()->unblock_handler) {
        thread->node()->unblock_handler(thread->node()->unblock_handler_ctx);
    }

    arch_irq_restore(flags);

    return E_OK;
}

void scheduler::handle_timer_irq()
{
    for (int p = NUM_PRIORITIES - 1; p >= 0; p++) {
        if (nullptr != blocked_queues_[p]) {
            intrusive_list *t = blocked_queues_[p];
            while (KTHREAD_NODE_PTR(t)->tsc_deadline < arch_tsc()) {
                auto next = t->next;
                unblock_thread(KTHREAD_PTR(t));
                if (next == t) {
                    break;
                }
                t = next;
            }
        }
    }
}

} // namespace otrix
