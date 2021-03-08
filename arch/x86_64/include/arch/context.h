#ifndef ARCH_CONTEXT_H
#define ARCH_CONTEXT_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "common/assert.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Arch-specific CPU context.
 */
struct arch_context
{
    uint64_t rbx;
    uint64_t r[4]; /** r12-r15 */
    uint64_t rflags;
    uint64_t rsp;
    uint64_t rbp;
};

/**
 * Save current CPU context into @c context.
 */
void arch_context_save(struct arch_context *context);

/**
 * Restore CPU context from @c context.
 */
void arch_context_restore(const struct arch_context *context);

void arch_context_switch(struct arch_context *prev, struct arch_context *next);

static inline void arch_context_setup(struct arch_context *ctx,
        uint64_t *stack, const size_t stack_size,
        void(*entry)(void *), void *thread_ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    // Copy current RFLAGS into the context RFLAGS
	asm volatile("pushfq ; popq %0;" : "=rm" (ctx->rflags) : : "memory");

    uint64_t *stack_ptr = (uint64_t *)((uint8_t *)stack + stack_size - 8 * 3);
    extern void arch_context_thread_entry(void);
    stack_ptr[0] = (uint64_t)arch_context_thread_entry;
    stack_ptr[1] = (uint64_t)thread_ctx;
    stack_ptr[2] = (uint64_t)entry;
    ctx->rsp = (uint64_t)stack_ptr;
}

#ifdef __cplusplus
}
#endif

#endif // ARCH_CONTEXT_H
