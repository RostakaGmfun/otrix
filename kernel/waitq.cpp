#include "kernel/waitq.hpp"
#include "kernel/kthread.hpp"
#include "arch/asm.h"
#include "otrix/immediate_console.hpp"

namespace otrix
{

waitq::waitq(): wq_(nullptr)
{

}

waitq::~waitq()
{
    notify_all();
}

void waitq::wait()
{
    kthread *thread = scheduler::get().get_current_thread();
    auto flags = arch_irq_save();
    scheduler::get().remove_thread(thread);
    if (nullptr == wq_) {
        intrusive_list_init(&thread->node()->list_node);
        wq_ = &thread->node()->list_node;
    } else {
        intrusive_list_push_back(wq_, &thread->node()->list_node);
    }
    arch_irq_restore(flags);

    scheduler::get().schedule();
}

void waitq::notify_one()
{
    kthread *thread = nullptr;
    auto flags = arch_irq_save();
    if (nullptr == wq_) {
        return arch_irq_restore(flags);
    }

    thread = KTHREAD_PTR(wq_);
    if (wq_->next == wq_) {
        wq_ = nullptr;
    } else {
        wq_ = wq_->next;
    }
    arch_irq_restore(flags);
    scheduler::get().add_thread(thread);
}

void waitq::notify_all()
{
    auto flags = arch_irq_save();
    while (wq_ != nullptr) {
        notify_one();
    }
    arch_irq_restore(flags);
}

} // namespace otrix
