.section .text
.code64
.global arch_context_save
arch_context_save:
    mov 8(%esp), %rax
    mov %rbx, (%rax)
    mov %r8, 8(%rax)
    mov %r9, 16(%rax)
    mov %r10, 24(%rax)
    mov %r11, 32(%rax)
    mov %r12, 40(%rax)
    mov %r13, 48(%rax)
    mov %r14, 56(%rax)
    mov %r15, 64(%rax)
    mov %rsi, 72(%rax)
    mov %rdi, 80(%rax)
    pushfq
    pop %rdx
    mov %rdx, 88(%rax)
    mov %rsp, 96(%rax)
    ret

.global arch_context_restore
arch_context_restore:
    // TODO
    ret
