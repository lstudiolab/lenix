ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)
SECTIONS
{
    . = 0xffffffff80000000;
    .text : ALIGN(4K) { KEEP(*(.text.boot)) *(.text .text.*) }
    .rodata : ALIGN(4K) { *(.rodata .rodata.*) }
    .limine_requests : ALIGN(8) { KEEP(*(.limine_requests)) }
    .data : ALIGN(4K) { *(.data .data.*) }
    .bss : ALIGN(4K) { *(COMMON) *(.bss .bss.*) }
    /DISCARD/ : { *(.eh_frame*) *(.note*) *(.comment*) }
}
