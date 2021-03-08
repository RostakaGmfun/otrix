.section .text
.code64
.global arch_context_save
arch_context_save:
    mov %rbx, (%rdi)
    mov %r12, 8(%rdi)
    mov %r13, 16(%rdi)
    mov %r14, 24(%rdi)
    mov %r15, 32(%rdi)
    pushfq
    pop %rdx
    mov %rdx, 40(%rdi)
    mov %rsp, 48(%rdi)
    mov %rbp, 56(%rdi)
    ret

.global arch_context_restore
arch_context_restore:
    mov (%rdi), %rbx
    mov 8(%rdi), %r12
    mov 16(%rdi), %r13
    mov 24(%rdi), %r14
    mov 32(%rdi), %r15
    mov 40(%rdi), %rdx
    push %rdx
    popfq
    mov 48(%rdi), %rsp
    mov 56(%rdi), %rbp
    ret

.global arch_context_switch
arch_context_switch:
    mov %rbx, (%rdi)
    mov %r12, 8(%rdi)
    mov %r13, 16(%rdi)
    mov %r14, 24(%rdi)
    mov %r15, 32(%rdi)
    pushfq
    pop %rdx
    mov %rdx, 40(%rdi)
    mov %rsp, 48(%rdi)
    mov %rbp, 56(%rdi)

    mov (%rsi), %rbx
    mov 8(%rsi), %r12
    mov 16(%rsi), %r13
    mov 24(%rsi), %r14
    mov 32(%rsi), %r15
    mov 40(%rsi), %rdx
    push %rdx
    popfq
    mov 48(%rsi), %rsp
    mov 56(%rsi), %rbp
    ret

.global arch_context_thread_entry
arch_context_thread_entry:
    pop %rdi
    ret
