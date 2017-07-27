#ifndef OTRIX_TASK_H
#define OTRIX_TASK_H

struct task_ctx
{
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rsp;
    uint64_t rbp;
    uint64_t rip;
    uint64_t rfalgs;
};

extern void save_ctx(task_ctx * const ctx);

#endif // OTRIX_TASK_H
