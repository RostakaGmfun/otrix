#include "kernel/timer_service.hpp"
#include "kernel/kthread.hpp"
#include "arch/kvmclock.hpp"
#include "arch/asm.h"

#define TASK_PTR(list_ptr) container_of(list_ptr, task_desc_t, list_node)

namespace otrix
{

static constexpr auto STACK_SIZE = 64 * 1024 / sizeof(uint64_t);
static constexpr auto TASK_POOL_SIZE = 32;

timer_service::timer_service(int priority, void *shared_ctx): pending_tasks_(nullptr),
                                            task_pool_(nullptr),
                                            task_pool_ptr_(nullptr),
                                            shared_ctx_(shared_ctx),
                                            service_thread_(new kthread(STACK_SIZE, [] (void *ctx) {
                                                                timer_service *p_this = (timer_service *)ctx;
                                                                p_this->run();
                                                            }, "timer_service", priority, this))
{
    task_pool_ptr_ = new task_desc_t[TASK_POOL_SIZE];
    task_pool_ = nullptr;
    for (size_t i = 0; i < TASK_POOL_SIZE; i++) {
        intrusive_list_init(&task_pool_ptr_[i].list_node);
        task_pool_ = intrusive_list_push_back(task_pool_, &task_pool_ptr_[i].list_node);
    }
}

timer_service::~timer_service()
{
    delete service_thread_;
    for (size_t i = 0; i < TASK_POOL_SIZE; i++) {
        intrusive_list_unlink_node(&task_pool_ptr_[i].list_node);
    }
    delete [] task_pool_ptr_;
}

timer_task_t *timer_service::add_task(timer_cb_t cb, void *ctx, uint64_t timeout_ms)
{
    if (nullptr == cb || nullptr == task_pool_) {
        return nullptr;
    }

    mutex_.lock();
    intrusive_list *task = task_pool_;
    task_pool_ = intrusive_list_delete(task_pool_, task);
    task_desc_t *p_desc = container_of(task, task_desc_t, list_node);
    p_desc->p_cb = cb;
    p_desc->p_ctx = ctx;
    p_desc->deadline_tsc = arch_tsc() + arch::kvmclock::ns_to_tsc(timeout_ms * 1000 * 1000);

    pending_tasks_ = intrusive_list_insert_sorted(pending_tasks_, task,
            [] (intrusive_list *a, intrusive_list *b)
            {
                return TASK_PTR(a)->deadline_tsc > TASK_PTR(b)->deadline_tsc;
            });
    const bool wakeup_service_thread = (task == pending_tasks_);
    mutex_.unlock();

    if (wakeup_service_thread) {
        // Wakeup thread to service the new task
        scheduler::get().wake(service_thread_);
    }

    return task;
}

void timer_service::remove_task(timer_task_t *p_task)
{
    if (nullptr != p_task) {
        mutex_.lock();
        pending_tasks_ = intrusive_list_delete(pending_tasks_, p_task);
        task_pool_ = intrusive_list_push_back(task_pool_, p_task);
        mutex_.unlock();
    }
}

void timer_service::run()
{
    // TODO: join with desctructor
    while (1) {
        uint64_t next_tsc_deadline = -1;

        // Execute pending tasks
        while (1) {
            mutex_.lock();
            if (nullptr == pending_tasks_) {
                mutex_.unlock();
                break;
            }
            if (TASK_PTR(pending_tasks_)->deadline_tsc > arch_tsc()) {
                next_tsc_deadline = TASK_PTR(pending_tasks_)->deadline_tsc;
                mutex_.unlock();
                break;
            }
            auto task = TASK_PTR(pending_tasks_);
            pending_tasks_ = intrusive_list_delete(pending_tasks_, pending_tasks_);
            mutex_.unlock();

            task->p_cb(task->p_ctx, shared_ctx_);
            memset(&task, 0, sizeof(task));

            mutex_.lock();
            task_pool_ = intrusive_list_push_back(task_pool_, &task->list_node);
            mutex_.unlock();
        }

        scheduler::get().sleep_until(next_tsc_deadline);
    }
}

} // namespace otrix
