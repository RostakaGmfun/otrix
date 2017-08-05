#ifndef OTRIX_ARCH_COMMON_H
#define OTRIX_ARCH_COMMON_H

__attribute__((noreturn)) static inline void arch_assert_panic(const char *file,
        const int line, const char *asserted_expr)
{
    // TODO(RostakaGmfun): Add debug print
    // when string formatting utils will be available
    for (;;);
}

#endif // OTRIX_ARCH_COMMON_H
