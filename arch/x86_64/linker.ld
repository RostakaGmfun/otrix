ENTRY(start)

SECTIONS {
    . = 1M;

    .text :
    {
        KEEP(*(.multiboot_header))
        *(.text)
    }
}
