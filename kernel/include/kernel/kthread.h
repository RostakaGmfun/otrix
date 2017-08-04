#ifndef KTHREAD_H
#define KTHREAD_H

#include "arch/context.h"
#include "common/error.h"

#define KTHREAD_STACK_SIZE 1024

/**
 * @file kthread.h
 *
 * Manages kernel single-core cooperative threads (fibers).
 */

struct kthread
{
    struct arch_context context;
    uint32_t id;
    // Temp solution until page allocation will be available
    uint8_t stack[KTHREAD_STACK_SIZE];
};

/**
 * Initialize the kthread structure.
 */
error_t kthread_create(struct kthread * const thread);

/**
 * Retrieve the id of currently running thread.
 */
uint32_t kthread_get_current_id();

/**
 * Suspend current thread allowing other threads to run.
 */
error_t kthread_yield();

/**
 * Destroy the thread with given @c id.
 */
error_t kthread_destroy(uint32_t id);

#endif // KTHREAD_H
