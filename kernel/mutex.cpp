#include "kernel/mutex.hpp"
#include "kernel/kthread.hpp"
#include "arch/asm.h"

namespace otrix
{

mutex::mutex(): owner_(nullptr)
{}

mutex::~mutex()
{
    waiting_queue_.notify_all();
}

bool mutex::lock(uint64_t timeout_ms)
{
    auto flags = arch_irq_save();

    if (nullptr == owner_) {
        owner_ = scheduler::get().get_current_thread();
        arch_irq_restore(flags);
        return true;
    }
    arch_irq_restore(flags);
    if (timeout_ms > 0) {
        if (waiting_queue_.wait(timeout_ms)) {
            owner_ = scheduler::get().get_current_thread();
            return true;
        }
    }
    return false;
}

void mutex::unlock()
{
    auto flags = arch_irq_save();
    if (scheduler::get().get_current_thread() == owner_) {
        if (!waiting_queue_.empty()) {
            waiting_queue_.notify_one();
        } else {
            owner_ = nullptr;
        }
    } else {
        // TODO: assert to enforce mutex ownership semantics
    }
    arch_irq_restore(flags);
}

} // namespace otrix
