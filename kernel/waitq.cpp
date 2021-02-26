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
    waitq_item ctx;
    intrusive_list_init(&ctx.wq_node);
    ctx.ret = 0;
    ctx.thread = thread;
    scheduler::get().block_thread(thread, -1, unblock_handler, &ctx);
    wq_ = intrusive_list_push_back(wq_, &ctx.wq_node);
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

    waitq_item *node = container_of(node, waitq_item, wq_node);
    thread = node->thread;
    wq_ = intrusive_list_delete(wq_, &ctx->wq_node);
    scheduler::get().unblock_thread(thread);
    arch_irq_restore(flags);
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
