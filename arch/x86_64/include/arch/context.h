#ifndef ARCH_CONTEXT_H
#define ARCH_CONTEXT_H

#include <stdint.h>
#include <stddef.h>
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
extern void arch_context_save(struct arch_context *context);

/**
 * Restore CPU context from @c context.
 */
extern void arch_context_restore(const struct arch_context *context);

static inline void arch_context_setup(struct arch_context *ctx,
        const void * const stack, const size_t stack_size,
        void(*entry)(void *), void * const params)
{
    kASSERT(ctx != NULL && entry != NULL && params != NULL);

    // Copy current RFLAGS into the context RFLAGS
    asm volatile ("pushfq \npop %%rax\nmov %%rax, %0\n" :"=r"(ctx->rflags) : :"%rax" );

    uint64_t *stack_ptr = (uint64_t *)((uint8_t *)stack + stack_size);
    // TODO(RostakaGmfun): Parameter passing
    stack_ptr[-1] = (uint64_t)entry;
    ctx->rsp = (uint64_t)stack_ptr - 8;
}

#ifdef __cplusplus
}
#endif

#endif // ARCH_CONTEXT_H
