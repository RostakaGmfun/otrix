#include "kernel/kthread.hpp"

#include <algorithm>
#include "kernel/alloc.hpp"
#include "arch/asm.h"
#include "arch/kvmclock.hpp"
#include "arch/lapic.hpp"

namespace otrix
{

kthread::kthread(size_t stack_size, kthread_entry entry, const char *name, int priority, void *ctx):
    stack_size_(stack_size), entry_(entry), node_(this), priority_(priority), name_(name)
{
    stack_ = new uint64_t[stack_size];
    arch_context_setup(&context_, stack_,
            stack_size, entry_, ctx);
}

kthread::kthread(const char *name, int priority):
    stack_(nullptr), stack_size_(0), entry_(nullptr), node_(this), priority_(priority), name_(name)
{
    memset(&context_, 0, sizeof(context_));
    intrusive_list_init(&node_.list_node);
}

kthread::~kthread()
{
    scheduler::get().remove_thread(this);
    delete [] stack_;
}

scheduler::scheduler(): current_thread_(), need_resched_(false), nearest_tsc_deadline_(-1),
                        idle_thread_("IDLE", 0), preempt_disable_(0)
{
    for (int i = 0; i < NUM_PRIORITIES; i++) {
        runnable_queues_[i] = nullptr;
    }
    for (int i = 0; i < NUM_PRIORITIES; i++) {
        blocked_queues_[i] = nullptr;
    }
    add_thread(&idle_thread_);
    current_thread_ = &idle_thread_.node()->list_node;
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
    thread->node()->state = KTHREAD_STATE_RUNNABLE;
    runnable_queues_[prio] = intrusive_list_push_back(runnable_queues_[prio],
            &thread->node()->list_node);
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
    runnable_queues_[prio] = intrusive_list_delete(runnable_queues_[prio],
            &thread->node()->list_node);
    arch_irq_restore(flags);
    return E_OK;
}

kerror_t scheduler::schedule()
{
    auto flags = arch_irq_save();
    if (preempt_disable_ > 0) {
        arch_irq_restore(flags);
        return E_OK;
    }

    need_resched_ = false;
    intrusive_list *prev_thread = current_thread_;

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
        arch_context_switch(KTHREAD_PTR(prev_thread)->context(),
                KTHREAD_PTR(current_thread_)->context());
    }

    arch_irq_restore(flags);

    return E_OK;
}

kerror_t scheduler::sleep(uint64_t block_time_ms)
{
    auto flags = arch_irq_save();

    kthread *thread = KTHREAD_PTR(current_thread_);

    const int priority = thread->priority();

    remove_thread(thread);
    thread->node()->state = KTHREAD_STATE_BLOCKED;
    if (static_cast<uint64_t>(-1) == block_time_ms) {
        thread->node()->tsc_deadline = -1;
        blocked_queues_[priority] = intrusive_list_push_back(blocked_queues_[priority], &thread->node()->list_node);
    } else {
        const uint64_t tsc_deadline = arch_tsc() + arch::kvmclock::ns_to_tsc(block_time_ms * 1000 * 1000);
        thread->node()->tsc_deadline = tsc_deadline;
        nearest_tsc_deadline_ = std::min(nearest_tsc_deadline_, tsc_deadline);
        if (nullptr != blocked_queues_[priority]) {
            blocked_queues_[priority] = intrusive_list_insert_sorted(blocked_queues_[priority],
                    &thread->node()->list_node,
                    [] (intrusive_list *a, intrusive_list *b)
                    {
                        return KTHREAD_NODE_PTR(a)->tsc_deadline > KTHREAD_NODE_PTR(b)->tsc_deadline;
                    });
            intrusive_list *t = blocked_queues_[priority];
            do {
                t = t->next;
            } while (t != blocked_queues_[priority]);
        } else {
            blocked_queues_[priority] = intrusive_list_push_back(blocked_queues_[priority], &thread->node()->list_node);
        }
    }

    if (nearest_tsc_deadline_ != static_cast<uint64_t>(-1)) {
        arch::local_apic::start_timer(nearest_tsc_deadline_);
    }

    schedule();

    arch_irq_restore(flags);

    return E_OK;
}

kerror_t scheduler::wake(kthread *thread)
{
    auto flags = arch_irq_save();

    if (thread->node()->state != KTHREAD_STATE_BLOCKED) {
        arch_irq_restore(flags);
        return E_INVAL;
    }

    blocked_queues_[thread->priority()] = intrusive_list_delete(blocked_queues_[thread->priority()], &thread->node()->list_node);

    add_thread(thread);

    setup_timer();

    arch_irq_restore(flags);

    return E_OK;
}

void scheduler::preempt_disable()
{
    auto flags = arch_irq_save();
    preempt_disable_++;
    arch_irq_restore(flags);
}

void scheduler::preempt_enable(bool reschedule)
{
    auto flags = arch_irq_save();
    if (preempt_disable_ > 0) {
        preempt_disable_--;
        if (0 == preempt_disable_ && reschedule && need_resched_) {
            schedule();
        }
    }
    arch_irq_restore(flags);
}

void scheduler::handle_timer_irq(void *p_ctx)
{
    (void)p_ctx;
    get().handle_timer_irq();
}

void scheduler::handle_timer_irq()
{
    nearest_tsc_deadline_ = static_cast<uint64_t>(-1);

    for (int p = NUM_PRIORITIES - 1; p >= 0; p--) {
        if (nullptr != blocked_queues_[p]) {
        }
        while (nullptr != blocked_queues_[p] && KTHREAD_NODE_PTR(blocked_queues_[p])->tsc_deadline < arch_tsc()) {
            auto thread = KTHREAD_PTR(blocked_queues_[p]);
            wake(thread);
        }
        if (nullptr != blocked_queues_[p]) {
            nearest_tsc_deadline_ = std::min(KTHREAD_NODE_PTR(blocked_queues_[p])->tsc_deadline,
                    nearest_tsc_deadline_);
        }
    }

    if (nearest_tsc_deadline_ != static_cast<uint64_t>(-1)) {
        arch::local_apic::start_timer(nearest_tsc_deadline_);
    }

    if (need_resched_) {
        schedule();
    }
}

void scheduler::setup_timer()
{
    nearest_tsc_deadline_ = static_cast<uint64_t>(-1);

    for (int p = NUM_PRIORITIES - 1; p >= 0; p--) {
        if (nullptr != get().blocked_queues_[p]) {
            get().nearest_tsc_deadline_ =
                std::min(KTHREAD_NODE_PTR(get().blocked_queues_[p])->tsc_deadline,
                        get().nearest_tsc_deadline_);
        }
    }

    if (get().nearest_tsc_deadline_ != static_cast<uint64_t>(-1)) {
        arch::local_apic::start_timer(get().nearest_tsc_deadline_);
    }

}

} // namespace otrix
