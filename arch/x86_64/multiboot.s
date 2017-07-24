.section .multiboot_header
header_start:
    // magic number (multiboot 2)
    .long 0xe85250d6
    // architecture 0 (protected mode i386)
    .long 0
    // header length
    .long header_end - header_start
    // checksum
    .long 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))

    // insert optional multiboot tags here

    // required end tag
    // type
    .short 0
    // flags
    .short 0
    // size
    .long 8
header_end:
