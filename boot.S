/* SolverUNIX x86_64: вход по протоколу Multiboot2.
 *
 * Загрузчик (Kolon Kiwi или GRUB) отдаёт управление в 32-битном protected mode:
 *   EAX = 0x36D76289, EBX = физ. адрес Multiboot2 info, paging выключен, IF=0.
 * Этот файл: проверка magic и long mode (CPUID) -> страницы (identity 0..1 GiB,
 * 2 MiB) -> PAE + EFER.LME + PG -> 64-битный GDT -> прыжок в 64-битный код -> kmain().
 * Файл целиком - ELF64; 32-битная заглушка - обычный код внутри него. */

.set MB2_MAGIC,      0xE85250D6
.set MB2_BOOT_MAGIC, 0x36D76289

.section .multiboot2, "a"
.align 8
mb2_header_start:
    .long MB2_MAGIC
    .long 0                                   /* архитектура: i386 protected mode */
    .long mb2_header_end - mb2_header_start
    .long (0x100000000 - (MB2_MAGIC + 0 + (mb2_header_end - mb2_header_start))) & 0xFFFFFFFF
    .short 0                                  /* end tag */
    .short 0
    .long 8
mb2_header_end:

.section .bss
.align 4096
pml4:  .skip 4096
pdpt:  .skip 4096
pd:    .skip 4096
.align 16
stack_bottom:
    .skip 16384
.global stack_top
stack_top:
mb_magic: .skip 4
mb_info:  .skip 4

.section .rodata
.align 8
gdt64:
    .quad 0
    .quad 0x00209A0000000000        /* 0x08: код, long mode, ring 0 */
    .quad 0x0000920000000000        /* 0x10: данные, ring 0 */
gdt64_ptr:
    .word 23
    .long gdt64

.section .text
.code32
.global _start
.type _start, @function
_start:
    cli
    cmpl $MB2_BOOT_MAGIC, %eax
    jne  .bad_boot
    movl %eax, mb_magic
    movl %ebx, mb_info
    movl $stack_top, %esp

    movl $0x80000000, %eax          /* long mode поддерживается? */
    cpuid
    cmpl $0x80000001, %eax
    jb   .no_long_mode
    movl $0x80000001, %eax
    cpuid
    testl $(1 << 29), %edx
    jz   .no_long_mode

    movl $pdpt, %eax                /* PML4[0] -> PDPT[0] -> PD (512 x 2 MiB = 1 GiB) */
    orl  $0x3, %eax
    movl %eax, pml4
    movl $pd, %eax
    orl  $0x3, %eax
    movl %eax, pdpt
    xorl %ecx, %ecx
1:  movl %ecx, %eax
    shll $21, %eax
    orl  $0x83, %eax                /* present | writable | huge (2 MiB) */
    movl %eax, pd(,%ecx,8)
    incl %ecx
    cmpl $512, %ecx
    jne  1b

    movl $pml4, %eax
    movl %eax, %cr3
    movl %cr4, %eax
    orl  $(1 << 5), %eax            /* PAE */
    movl %eax, %cr4
    movl $0xC0000080, %ecx          /* EFER */
    rdmsr
    orl  $(1 << 8), %eax            /* LME */
    wrmsr
    movl %cr0, %eax
    orl  $(1 << 31), %eax           /* PG */
    movl %eax, %cr0

    lgdt gdt64_ptr
    ljmp $0x08, $long_mode_start

.bad_boot:                          /* "MB2" красным: загрузчик не Multiboot2 */
    movl $0x4F424F4D, 0xB8000
    movw $0x4F32, 0xB8004
    jmp  .halt32
.no_long_mode:                      /* "NO64" красным */
    movl $0x4F4F4F4E, 0xB8000
    movl $0x4F344F36, 0xB8004
.halt32:
    hlt
    jmp  .halt32

.code64
long_mode_start:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    movq $stack_top, %rsp
    xorl %ebp, %ebp
    movl mb_magic(%rip), %edi
    movl mb_info(%rip), %esi
    call kmain
3:  cli
    hlt
    jmp  3b

.section .note.GNU-stack,"",@progbits
