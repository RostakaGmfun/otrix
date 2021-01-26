ENTRY(start)

SECTIONS {
    . = 1M;

    .text :
    {
        KEEP(*(.multiboot_header))
        *(.text)
    }

    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP (*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    }

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    }

    .data :
    {
        *(*)
    }

    __binary_end = .;
}
