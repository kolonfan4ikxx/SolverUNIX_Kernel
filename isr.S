/* Заглушки исключений (0-31), IRQ (32-47) и системного вызова (0x80) */
.section .data
.global isr_table
.align 8
isr_table:

.section .text

.macro ISR n, has_err
.section .text
.global isr\n
isr\n:
.if \has_err == 0
    pushq $0
.endif
    pushq $\n
    jmp  isr_common
.section .data
    .quad isr\n
.section .text
.endm

ISR 0, 0
ISR 1, 0
ISR 2, 0
ISR 3, 0
ISR 4, 0
ISR 5, 0
ISR 6, 0
ISR 7, 0
ISR 8, 1
ISR 9, 0
ISR 10, 1
ISR 11, 1
ISR 12, 1
ISR 13, 1
ISR 14, 1
ISR 15, 0
ISR 16, 0
ISR 17, 1
ISR 18, 0
ISR 19, 0
ISR 20, 0
ISR 21, 0
ISR 22, 0
ISR 23, 0
ISR 24, 0
ISR 25, 0
ISR 26, 0
ISR 27, 0
ISR 28, 0
ISR 29, 0
ISR 30, 1
ISR 31, 0
ISR 32, 0
ISR 33, 0
ISR 34, 0
ISR 35, 0
ISR 36, 0
ISR 37, 0
ISR 38, 0
ISR 39, 0
ISR 40, 0
ISR 41, 0
ISR 42, 0
ISR 43, 0
ISR 44, 0
ISR 45, 0
ISR 46, 0
ISR 47, 0

.global isr128
isr128:
    pushq $0
    pushq $0x80
    jmp  isr_common

/* Стек на входе (сверху вниз): int_no, err_code, rip, cs, rflags, rsp, ss.
 * После 15 push-ей rsp выровнен на 16 -> можно вызывать C. */
isr_common:
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq %rsp, %rdi               /* struct regs* */
    cld
    call isr_handler
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax
    addq $16, %rsp                /* int_no + err_code */
    iretq

.global gdt_flush
gdt_flush:                        /* rdi = struct gdt_ptr* */
    lgdt (%rdi)
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    pushq $0x08
    leaq 1f(%rip), %rax
    pushq %rax
    lretq
1:  ret

.global tss_flush
tss_flush:                        /* rdi = селектор TSS */
    ltr  %di
    ret

.global idt_flush
idt_flush:                        /* rdi = struct idt_ptr* */
    lidt (%rdi)
    ret

.section .note.GNU-stack,"",@progbits
