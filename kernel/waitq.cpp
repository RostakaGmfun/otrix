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
    ctx.wakeup_successful = true;
    ctx.thread = thread;
    ctx.self = this;
    wq_ = intrusive_list_push_back(wq_, &ctx.list_node);
    scheduler::get().block_thread(thread, timeout_ms, unblock_handler, &ctx);
    arch_irq_restore(flags);

    scheduler::get().schedule();
    return ctx.wakeup_successful;
}

void waitq::notify_one()
{
    kthread *thread = nullptr;
    auto flags = arch_irq_save();
    if (nullptr == wq_) {
        return arch_irq_restore(flags);
    }

    waitq_item *ctx = container_of(wq_, waitq_item, list_node);
    thread = ctx->thread;
    wq_ = intrusive_list_delete(wq_, &ctx->list_node);
    scheduler::get().unblock_thread(thread);
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

void waitq::unblock_handler(void *ctx)
{
    auto flags = arch_irq_save();
    waitq_item *item = static_cast<waitq_item *>(ctx);
    waitq *p_this = item->self;
    item->wakeup_successful = false;
    p_this->wq_ = intrusive_list_delete(p_this->wq_, &item->list_node);
    arch_irq_restore(flags);
}

} // namespace otrix
