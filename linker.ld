/* SolverUNIX x86_64: настоящий ELF64 (ET_EXEC), загружается по p_paddr от 1 MiB.
 * Три PT_LOAD сегмента с явными правами (W^X). ELF-заголовок в загружаемую
 * память не попадает; Multiboot2-заголовок лежит в начале первого сегмента
 * (файловое смещение << 32 KiB, выровнено на 8). */
ENTRY(_start)

PHDRS
{
    text   PT_LOAD FLAGS(5);    /* R+X */
    rodata PT_LOAD FLAGS(4);    /* R   */
    data   PT_LOAD FLAGS(6);    /* R+W (включая .bss: memsz > filesz) */
}

SECTIONS
{
    . = 1M;

    .boot : { KEEP(*(.multiboot2)) } :text
    .text : { *(.text .text.*) }     :text

    . = ALIGN(4K);
    .rodata : { *(.rodata .rodata.*) } :rodata

    . = ALIGN(4K);
    .data : { *(.data .data.*) }     :data
    .bss  : { *(COMMON) *(.bss .bss.*) } :data

    . = ALIGN(4K);
    kernel_end = .;

    /DISCARD/ : { *(.comment) *(.eh_frame*) *(.note*) }
}
