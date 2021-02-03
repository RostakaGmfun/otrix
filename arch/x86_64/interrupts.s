.section .text
.code64
.global arch_unused_irq_handler
arch_unused_irq_handler:
    iretq

.extern irq_manager_irq_handler
.global arch_used_irq_handler
arch_used_irq_handler:
   push %rax
   push %rcx
   push %rdx
   push %rdi
   push %rsi
   push %r8
   push %r9
   push %r10
   push %r11
   call irq_manager_irq_handler
   pop %r11
   pop %r10
   pop %r9
   pop %r8
   pop %rsi
   pop %rdi
   pop %rdx
   pop %rcx
   pop %rax
   iretq

.global arch_exception_handler
arch_exception_handler:
    jmp .
