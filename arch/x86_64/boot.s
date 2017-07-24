.section .multiboot_header
header_start:
    // magic number (multiboot 2)
    .long 0xe85250d6
    // architecture 0 (protected mode i386)
    .long 0
    // header length
    .long header_end - header_start
    // checksum
    .long -(0xe85250d6 + 0 + (header_end - header_start))

    // insert optional multiboot tags here

    // required end tag
    // type
    .short 0
    // flags
    .short 0
    // size
    .long 8
header_end:

.extern kmain
.section .text
.code32
.global start
start:
    mov $stack_top, %esp

    call check_multiboot
    call check_cpuid
    call check_long_mode

    call set_up_page_tables
    call enable_paging

    lgdt gdt64_pointer
    ljmp $0x08, $long_mode_start

check_multiboot:
    cmpl $0x36d76289, %eax
    jne .no_multiboot
    ret
.no_multiboot:
    mov $'0', %al
    jmp error

check_cpuid:
    // Check if CPUID is supported by attempting to flip the ID bit (bit 21)
    // in the FLAGS register. If we can flip it, CPUID is available.

    // Copy FLAGS in to EAX via stack
    pushfl
    pop %eax

    // Copy to ECX as well for comparing later on
    mov %eax, %ecx

    // Flip the ID bit
    xor $(1 << 21), %eax

    // Copy EAX to FLAGS via the stack
    push %eax
    popfl

    // Copy FLAGS back to EAX (with the flipped bit if CPUID is supported)
    pushfl
    pop %eax

    // Restore FLAGS from the old version stored in ECX (i.e. flipping the
    // ID bit back if it was ever flipped).
    push %ecx
    popfl

    // Compare EAX and ECX. If they are equal then that means the bit
    // wasn't flipped, and CPUID isn't supported.
    cmp %ecx, %eax
    je .no_cpuid
    ret
.no_cpuid:
    mov $'1', %al
    jmp error

check_long_mode:
    // test if extended processor info in available
    // implicit argument for cpuid
    movl $0x80000000, %eax
    // get highest supported argument
    cpuid
    // it needs to be at least 0x80000001
    cmpl $0x80000001, %eax
    // if it's less, the CPU is too old for long mode
    jb .no_long_mode

    // use extended info to test if long mode is available
    movl $0x80000001, %eax
    cpuid
    testl $(1 << 29), %edx
    jz .no_long_mode
    ret
.no_long_mode:
    movb $'2', %al
    jmp error

set_up_page_tables:
    // map first P4 entry to P3 table
    movl $p3_table, %eax
    // present + writable
    or $0b11, %eax
    movl %eax, p4_table

    // map first P3 entry to P2 table
    movl $p2_table, %eax
    or  $0b11, %eax
    movl %eax, p3_table

    // map each P2 entry to a huge 2MiB page
    // ecx - counter
    movl $0, %ecx

.map_p2_table:
    // map ecx-th P2 entry to a huge page that starts at address 2MiB*ecx
    // 2 MB
    movl $0x200000, %eax
    // start address of ecx-th page
    mul %ecx
    // present + writable + huge
    or $0b10000011, %eax
    // map ecx-th entry
    mov $p2_table, %edi
    mov %eax, (%edi,%ecx,8)

    inc %ecx
    // if counter == 512, the whole P2 table is mapped
    cmpl $512, %ecx
    // else map the next entry
    jne .map_p2_table

    ret

enable_paging:
    // load P4 to cr3 register (cpu uses this to access the P4 table)
    movl $p4_table, %eax
    movl %eax, %cr3

    // enable PAE-flag in cr4 (Physical Address Extension)
    movl  %cr4, %eax
    or $(1 << 5), %eax
    movl %eax, %cr4

    // set the long mode bit in the EFER MSR (model specific register)
    movl $0xC0000080, %ecx
    rdmsr
    or $(1 << 8), %eax
    wrmsr

    // enable paging in the cr0 register
    movl %cr0, %eax
    or $(1 << 31), %eax
    movl %eax, %cr0

    ret

// Prints `ERR: ` and the given error code to screen and hangs.
// parameter: error code (in ascii) in al
error:
    movl $0x4f524f45, 0xb8000
    movl $0x4f3a4f52, 0xb8004
    movl $0x4f204f20, 0xb8008
    movb %al, 0xb800a
    hlt

.section .bss
.align 4096
p4_table:
    .space 4096
p3_table:
    .space 4096
p2_table:
    .space 4096
stack_bottom:
    .space 1024
stack_top:
.section .rodata
gdt64:
    // zero entry
    .quad 0
.code:
    // code
    .quad (1<<43) | (1<<44) | (1<<47) | (1<<53)
gdt64_pointer:
    .hword gdt64_pointer - gdt64 - 1
    .quad gdt64

.section .text
.code64
long_mode_start:
    cli
    mov $0x2f592f412f4b2f4f, %rax
    mov %rax, 0xb8000
    call kmain
