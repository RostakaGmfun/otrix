#ifndef KTHREAD_H
#define KTHREAD_H

#include "arch/context.h"
#include "common/error.h"
#include "common/list.h"

#define KTHREAD_STACK_SIZE 1024

/**
 * @file kthread.h
 *
 * Manages kernel single-core cooperative threads (fibers).
 */

/**
 * Kernel thread context
 */
struct kthread
{
    struct intrusive_list list_entry;
    struct arch_context context;
    uint32_t id;
    // Temp solution until page allocation will be available
    uint8_t stack[KTHREAD_STACK_SIZE];
};

typedef void (*kthread_entry_t)(void *params);

/**
 * Initialize the kthread structure.
 */
error_t kthread_create(struct kthread * const thread,
        const kthread_entry_t *entry, void * const params);

/**
 * Retrieve the id of currently running thread.
 */
uint32_t kthread_get_current_id(void);

/**
 * Suspend current thread allowing other threads to run.
 */
error_t kthread_yield(void);

/**
 * Destroy the thread with given @c id.
 */
error_t kthread_destroy(uint32_t id);

/**
 * Starts the kernel thread scheduler.
 */
error_t kthread_schedule(void);

#endif // KTHREAD_H
