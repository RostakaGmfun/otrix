#ifndef ARCH_CONTEXT_H
#define ARCH_CONTEXT_H

#include <stdint.h>
#include <stddef.h>
#include "common/assert.h"

/**
 * Arch-specific CPU context.
 */
struct arch_context
{
    uint64_t rbx;
    uint64_t r[8]; /** r8-r15 */
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rflags;
    uint64_t rsp;
};

/**
 * Save current CPU context into @c context.
 */
extern void arch_context_save(struct arch_context *context);

/**
 * Restore CPU context from @c context.
 */
extern void arch_context_restore(const struct arch_context *context);

static inline void arch_context_setup(struct arch_context *ctx,
        const void * const stack, const size_t stack_size,
        const void * const entry, void * const params)
{
    kASSERT(ctx != NULL && entry != NULL && params != NULL);

    // Copy current RFLAGS into the context RFLAGS
    asm volatile ("pushq\npop %%rax\nmov %%rax, %0\n" :"=r"(ctx->rflags) : :"%rax" );

    ctx->rsp = (uint64_t)stack + stack_size;
    uint64_t *stack_ptr = (uint64_t *)stack;
    stack_ptr[stack_size] = (uint64_t)entry;
    stack_ptr[stack_size + 1] = (uint64_t)params;
}

#endif // ARCH_CONTEXT_H
