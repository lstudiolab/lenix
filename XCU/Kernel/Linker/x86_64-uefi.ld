ENTRY(_start_uefi)
OUTPUT_FORMAT(elf64-x86-64)
PHDRS
{
    text PT_LOAD FLAGS(5);
    rodata PT_LOAD FLAGS(4);
    data PT_LOAD FLAGS(6);
}
SECTIONS
{
    . = 0xffffffff80000000;
    .text : ALIGN(4K) { KEEP(*(.text.boot)) *(.text .text.*) } :text
    .rodata : ALIGN(4K) { *(.rodata .rodata.*) } :rodata
    .data : ALIGN(4K) { *(.data .data.*) } :data
    .bss : ALIGN(4K) { *(COMMON) *(.bss .bss.*) } :data
    /DISCARD/ : { *(.eh_frame*) *(.note*) *(.comment*) }
}
