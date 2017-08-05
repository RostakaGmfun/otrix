#include "kernel/kthread.h"
#include "common/assert.h"

#define list_to_kthread(list) intrusive_list_entry(list, struct kthread, list_entry)

/**
 * Kernel thread scheduler context
 */
static struct
{
    struct intrusive_list *threads;
    struct intrusive_list *current_thread;
} kthread_scheduler;

static bool kthread_find_by_id_pred(const struct intrusive_list *n,
        void *id)
{
    return intrusive_list_entry(n, struct kthread, list_entry)->id
        == *(uint32_t *)id;
}

static inline void kthread_run_current()
{
    arch_context_restore(&(list_to_kthread(kthread_scheduler.current_thread)->context));
}

error_t kthread_create(struct kthread * const thread,
        const kthread_entry_t *entry, void * const params)
{
    if (thread == NULL || entry == NULL) {
        return EINVAL;
    }

    intrusive_list_init(&thread->list_entry);

    arch_context_setup(&thread->context, thread->stack,
            sizeof(thread->stack), entry, params);

    if (kthread_scheduler.threads == NULL) {
        kthread_scheduler.threads = &thread->list_entry;
    } else {
        intrusive_list_push_back(kthread_scheduler.threads,
                &thread->list_entry);
    }

    return EOK;
}

uint32_t kthread_get_current_id(void)
{
    kASSERT(kthread_scheduler.current_thread != NULL);
    return list_to_kthread(kthread_scheduler.current_thread)->id;
}

error_t kthread_yield(void)
{
    kASSERT(kthread_scheduler.current_thread != NULL);
    arch_context_save(&list_to_kthread(kthread_scheduler.current_thread)->context);
    kthread_schedule();
    kthread_run_current();
}

error_t kthread_destroy(uint32_t id)
{
    kASSERT(kthread_scheduler.threads != NULL);
    struct intrusive_list *to_destroy =
        intrusive_list_find_first(kthread_scheduler.threads,
                kthread_find_by_id_pred, &id);
    if (to_destroy == NULL) {
        return EINVAL;
    }

    if (to_destroy == kthread_scheduler.current_thread) {
        kthread_schedule();
    }

    intrusive_list_unlink_node(to_destroy);
    kthread_run_current();
    return EOK;
}

error_t kthread_schedule(void)
{
    kASSERT(kthread_scheduler.current_thread != NULL);
    kASSERT(kthread_scheduler.threads != NULL);

    // Simple round-robin
    kthread_scheduler.current_thread =
        kthread_scheduler.current_thread->next;
    return EOK;
}
