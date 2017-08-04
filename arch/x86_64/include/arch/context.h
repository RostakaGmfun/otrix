#ifndef ARCH_CONTEXT_H
#define ARCH_CONTEXT_H

#include <stdint.h>

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

#endif // ARCH_CONTEXT_H
