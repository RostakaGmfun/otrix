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

bool waitq::wait(uint64_t timeout_ms)
{
    kthread *thread = scheduler::get().get_current_thread();
    auto flags = arch_irq_save();
    waitq_item ctx;
    intrusive_list_init(&ctx.list_node);
    ctx.wakeup_successful = false;
    ctx.thread = thread;
    wq_ = intrusive_list_push_back(wq_, &ctx.list_node);
    scheduler::get().sleep(timeout_ms);
    arch_irq_restore(flags);

    return ctx.wakeup_successful;
}

void waitq::notify_one()
{
    kthread *thread = nullptr;
    auto flags = arch_irq_save();
    if (nullptr == wq_) {
        arch_irq_restore(flags);
        return;
    }

    waitq_item *ctx = container_of(wq_, waitq_item, list_node);
    thread = ctx->thread;
    wq_ = intrusive_list_delete(wq_, &ctx->list_node);
    ctx->wakeup_successful = true;
    scheduler::get().wake(thread);
    arch_irq_restore(flags);
    scheduler::get().schedule();
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
