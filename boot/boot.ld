/* Плоский бинарник для 16/32-битного кода загрузчика.
 * Адрес загрузки передаётся: ld --defsym=__org=0x7C00 */
ENTRY(_start)
SECTIONS
{
    . = __org;
    .text : { *(.text .text.*) }
    /DISCARD/ : { *(.note*) *(.comment) *(.eh_frame*) *(.data*) *(.bss*) *(.rodata*) }
}
