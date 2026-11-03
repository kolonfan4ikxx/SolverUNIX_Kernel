/* Kolon Kiwi, stage 1 (MBR, 512 байт).
 * Через INT 13h/42h (LBA) грузит stage2 в 0x8000 и передаёт ему управление.
 * DL = BIOS-номер загрузочного диска (сохраняется и передаётся дальше). */
.include "kk.inc"

.code16
.section .text
.global _start
_start:
    cli
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $0x7C00, %sp
    sti
    cld
    movb %dl, boot_drive

    movw $msg_boot, %si
    call puts

    movb $0x41, %ah                 # есть ли LBA-расширения?
    movw $0x55AA, %bx
    movb boot_drive, %dl
    int  $0x13
    jc   no_lba
    cmpw $0xAA55, %bx
    jne  no_lba

    movb $0x42, %ah                 # читаем stage2
    movb boot_drive, %dl
    movw $dap, %si
    int  $0x13
    jc   read_fail

    movb boot_drive, %dl
    ljmp $0x0000, $KK_STAGE2_ADDR

no_lba:
    movw $msg_nolba, %si
    jmp  die
read_fail:
    movw $msg_read, %si
die:
    call puts
1:  hlt
    jmp  1b

puts:
    lodsb
    testb %al, %al
    jz   1f
    movb $0x0E, %ah
    movw $0x0007, %bx
    int  $0x10
    jmp  puts
1:  ret

boot_drive: .byte 0
msg_boot:   .asciz "Kolon Kiwi\r\n"
msg_nolba:  .asciz "No LBA BIOS\r\n"
msg_read:   .asciz "Stage2 read error\r\n"

.align 4
dap:
    .byte 0x10, 0
    .word KK_STAGE2_SECTORS
    .word KK_STAGE2_ADDR, 0
    .quad KK_STAGE2_LBA

.org 446                            # если код больше - ассемблер выдаст ошибку
.org 510
.word 0xAA55
