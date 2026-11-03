/* Kolon Kiwi, stage 2.
 *
 *  1. real mode: A20, карта памяти (E820), поиск ОС на всех BIOS-дисках 0x80..0x87,
 *     меню, чтение выбранного ядра (ELF64 как есть) в staging-область 8 MiB
 *     (INT 13h/42h в bounce-буфер + INT 15h/87h для копирования выше 1 MiB).
 *  2. protected mode (32 бит, flat): проверка ELF64, копирование PT_LOAD сегментов
 *     по p_paddr, сборка Multiboot2 info.
 *  3. передача управления по Multiboot2: EAX=0x36D76289, EBX=адрес info,
 *     прыжок на e_entry (32-битная заглушка ядра сама включает long mode).
 */
.include "kk.inc"

.set SECTOR_BUF,  0xC000            # буфер одного сектора
.set MENU_ARR,    0xC400            # записи меню
.set MENU_REC,    136               # 1 байт диск + 7 pad + 128 байт запись ОС
.set MENU_MAX,    9
.set BOUNCE_SEG,  0x1000            # bounce-буфер 0x10000 (32 KiB, не пересекает 64K границу)
.set E820_SEG,    0x2000            # E820-записи по 24 байта: 0x20000
.set E820_BUF,    0x20000
.set KSTAGE,      0x800000          # staging ELF-файла (8 MiB, влезает в 24-бит. базу INT15/87)
.set KMAX_BYTES,  0x600000
.set MB2_INFO,    0x50000
.set PM_STACK,    0x70000
/* смещения внутри записи меню */
.set REC_NAME,    8
.set REC_KLBA,    72
.set REC_KBYTES,  76
.set REC_CMDLINE, 80

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

    movw $0x0003, %ax               # текстовый режим 80x25, очистка
    int  $0x10
    movw $msg_banner, %si
    call puts

    call enable_a20
    call detect_memory
    call scan_disks
    call menu
    call load_kernel
    jmp  enter_protected

/* ---------------------------------------------------------------- вывод */
puts:                               # SI = ASCIZ
    lodsb
    testb %al, %al
    jz   1f
    call putc
    jmp  puts
1:  ret

putc:                               # AL = символ
    pushw %bx
    movb $0x0E, %ah
    movw $0x0007, %bx
    int  $0x10
    popw %bx
    ret

die:                                # SI = сообщение
    call puts
1:  hlt
    jmp  1b

/* ---------------------------------------------------------------- A20 */
enable_a20:
    movw $0x2401, %ax
    int  $0x15
    inb  $0x92, %al
    testb $2, %al
    jnz  1f
    orb  $2, %al
    andb $0xFE, %al
    outb %al, $0x92
1:  ret

/* ---------------------------------------------------------------- E820 */
detect_memory:
    movw $E820_SEG, %ax
    movw %ax, %es
    xorw %di, %di
    xorl %ebx, %ebx
    movw $0, e820_count
1:  movl $1, %es:20(%di)
    movl $0xE820, %eax
    movl $0x534D4150, %edx
    movl $24, %ecx
    int  $0x15
    jc   3f
    cmpl $0x534D4150, %eax
    jne  3f
    cmpl $0, %es:8(%di)             # пропускаем записи нулевой длины
    jne  2f
    cmpl $0, %es:12(%di)
    je   4f
2:  addw $24, %di
    incw e820_count
    cmpw $128, e820_count
    jae  3f
4:  testl %ebx, %ebx
    jnz  1b
3:  xorw %ax, %ax
    movw %ax, %es
    cmpw $0, e820_count
    jne  5f
    movw $msg_e820, %si
    jmp  die
5:  ret

/* ---------------------------------------------------------------- диски */
disk_read:                          # DAP заполнен, cur_drive задан; CF=1 при ошибке
    movb cur_drive, %dl
    movw $dap, %si
    movb $0x42, %ah
    int  $0x13
    ret

scan_disks:                         # ищем таблицы ОС на дисках 0x80..0x87
    movw $0, menu_count
    movb $0x80, scan_drive
1:  movb scan_drive, %al
    movb %al, cur_drive
    movw $1, dap_count
    movw $SECTOR_BUF, dap_off
    movw $0, dap_seg
    movl $KK_OSTBL_LBA, dap_lba
    movl $0, dap_lba+4
    call disk_read
    jc   6f
    cmpl $0x534F4B4B, SECTOR_BUF    # "KKOS"
    jne  6f
    cmpl $0x314C4254, SECTOR_BUF+4  # "TBL1"
    jne  6f
    movl SECTOR_BUF+8, %ecx         # число записей
    cmpl $3, %ecx
    jbe  2f
    movl $3, %ecx
2:  movw $SECTOR_BUF+16, %si
3:  testl %ecx, %ecx
    jz   6f
    cmpw $MENU_MAX, menu_count
    jae  6f
    pushl %ecx
    movw menu_count, %ax
    movw $MENU_REC, %dx
    mulw %dx
    addw $MENU_ARR, %ax
    movw %ax, %di
    movb scan_drive, %al
    movb %al, (%di)                 # какой это диск
    addw $8, %di
    movw $64, %cx
    rep  movsw                      # 128 байт записи ОС
    incw menu_count
    popl %ecx
    decl %ecx
    jmp  3b
6:  incb scan_drive
    cmpb $0x88, scan_drive
    jb   1b
    ret

/* ---------------------------------------------------------------- меню */
menu:
    cmpw $0, menu_count
    jne  1f
    movw $msg_noos, %si
    jmp  die
1:  movw $msg_menu, %si
    call puts

    movw $0, default_idx            # по умолчанию: первая ОС с загрузочного диска
    xorw %cx, %cx
    movw $MENU_ARR, %di
2:  cmpw menu_count, %cx
    jae  4f
    movb (%di), %al
    cmpb boot_drive, %al
    je   3f
    incw %cx
    addw $MENU_REC, %di
    jmp  2b
3:  movw %cx, default_idx

4:  xorw %cx, %cx                   # печать списка
    movw $MENU_ARR, %di
5:  cmpw menu_count, %cx
    jae  8f
    movw $msg_lb, %si
    call puts
    movb %cl, %al
    addb $0x31, %al                 # '1' + индекс
    call putc
    movw $msg_rb, %si
    call puts
    leaw REC_NAME(%di), %si
    call puts                       # название ОС
    movb (%di), %al
    cmpb boot_drive, %al
    je   7f
    movw $msg_other, %si            # " on another Disk (on /dev/sd"
    call puts
    movb (%di), %al
    subb $0x80, %al
    addb $0x61, %al                 # 0x80 -> 'a', 0x81 -> 'b', ...
    call putc
    movw $msg_close, %si            # ")"
    call puts
7:  movw $msg_crlf, %si
    call puts
    incw %cx
    addw $MENU_REC, %di
    jmp  5b

8:  movw $msg_prompt, %si
    call puts
    xorb %ah, %ah                   # ticks на старте
    int  $0x1A
    movw %dx, start_ticks
9:  movb $1, %ah                    # есть нажатие?
    int  $0x16
    jnz  10f
    xorb %ah, %ah
    int  $0x1A
    subw start_ticks, %dx
    cmpw $91, %dx                   # ~5 секунд (18.2 тика/с)
    jb   9b
    movw default_idx, %ax
    jmp  12f
10: xorb %ah, %ah
    int  $0x16
    cmpb $0x0D, %al                 # Enter
    jne  11f
    movw default_idx, %ax
    jmp  12f
11: subb $0x31, %al
    jb   9b
    xorb %ah, %ah
    cmpw menu_count, %ax
    jae  9b
12: movw %ax, sel_idx
    movw $MENU_REC, %dx
    mulw %dx
    addw $MENU_ARR, %ax
    movw %ax, sel_rec
    movw %ax, %di
    movb (%di), %al
    movb %al, sel_drive
    movw $msg_crlf, %si
    call puts
    movw $msg_booting, %si
    call puts
    movw sel_rec, %di
    leaw REC_NAME(%di), %si
    call puts
    movw $msg_crlf, %si
    call puts
    ret

/* ------------------------------------------------- чтение ядра (ELF64 целиком) */
load_kernel:
    movw sel_rec, %si
    movl REC_KLBA(%si), %eax
    movl %eax, k_lba
    movl REC_KBYTES(%si), %eax
    movl %eax, file_bytes
    testl %eax, %eax
    jz   bad_size
    cmpl $KMAX_BYTES, %eax
    ja   bad_size
    addl $511, %eax
    shrl $9, %eax
    movl %eax, k_left
    movl $KSTAGE, k_dest
    movb sel_drive, %al
    movb %al, cur_drive
1:  movl k_left, %eax
    testl %eax, %eax
    jz   3f
    cmpl $64, %eax
    jbe  2f
    movl $64, %eax
2:  movw %ax, chunk
    movw %ax, dap_count
    movw $0, dap_off
    movw $BOUNCE_SEG, dap_seg
    movl k_lba, %eax
    movl %eax, dap_lba
    movl $0, dap_lba+4
    call disk_read
    jc   disk_err
    movl k_dest, %eax               # INT 15h/87h: bounce -> k_dest
    movw %ax, gdt87_dst_lo
    shrl $16, %eax
    movb %al, gdt87_dst_mid
    movw chunk, %cx
    shlw $8, %cx                    # слов = секторов * 256
    xorw %ax, %ax
    movw %ax, %es
    movw $gdt87, %si
    movb $0x87, %ah
    int  $0x15
    jc   copy_err
    testb %ah, %ah
    jnz  copy_err
    movzwl chunk, %eax
    addl %eax, k_lba
    subl %eax, k_left
    shll $9, %eax
    addl %eax, k_dest
    movb $0x2E, %al                 # '.'
    call putc
    jmp  1b
3:  movw $msg_crlf, %si
    call puts
    ret
bad_size:
    movw $msg_badsize, %si
    jmp  die
disk_err:
    movw $msg_diskerr, %si
    jmp  die
copy_err:
    movw $msg_copyerr, %si
    jmp  die

/* ------------------------------------------------- переход в protected mode */
enter_protected:
    cli
    lgdtl gdt_ptr
    movl %cr0, %eax
    orl  $1, %eax
    movl %eax, %cr0
    ljmpl $0x08, $pm_entry

/* ================================================================ 32 бита */
.code32
pm_entry:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    movl $PM_STACK, %esp
    cld

    movl 0x000500, %edx             # проверка A20: 0x500 не должен совпадать с 0x100500
    movl $0xA5A5A5A5, 0x000500
    movl $0x5A5A5A5A, 0x100500
    cmpl $0xA5A5A5A5, 0x000500
    movl %edx, 0x000500
    jne  pm_a20_fail

    movl $KSTAGE, %esi              # ---- разбор ELF64
    cmpl $0x464C457F, (%esi)        # "\x7FELF"
    jne  pm_elf_bad
    cmpb $2, 4(%esi)                # ELFCLASS64
    jne  pm_elf_bad
    cmpb $1, 5(%esi)                # little endian
    jne  pm_elf_bad
    cmpw $2, 16(%esi)               # ET_EXEC
    jne  pm_elf_bad
    cmpw $62, 18(%esi)              # EM_X86_64
    jne  pm_elf_bad
    cmpl $0, 28(%esi)               # e_entry < 4 GiB (вход 32-битный)
    jne  pm_elf_bad
    cmpl $0, 36(%esi)               # e_phoff < 4 GiB
    jne  pm_elf_bad
    movl 24(%esi), %eax
    cmpl $0x100000, %eax
    jb   pm_elf_bad
    movl %eax, entry_point
    movl 32(%esi), %ebx
    addl %esi, %ebx                 # EBX = таблица program headers
    movzwl 54(%esi), %edx           # e_phentsize
    movzwl 56(%esi), %ebp           # e_phnum
    cmpl $56, %edx
    jb   pm_elf_bad
    testl %ebp, %ebp
    jz   pm_elf_bad

pm_ph_loop:
    cmpl $1, (%ebx)                 # PT_LOAD?
    jne  pm_ph_next
    movl 12(%ebx), %eax             # старшие половины offset/paddr/filesz/memsz = 0
    orl  28(%ebx), %eax
    orl  36(%ebx), %eax
    orl  44(%ebx), %eax
    jnz  pm_elf_bad
    movl 24(%ebx), %edi             # p_paddr
    cmpl $0x100000, %edi            # только выше 1 MiB
    jb   pm_elf_bad
    movl 40(%ebx), %ecx             # p_memsz
    movl %edi, %eax
    addl %ecx, %eax
    jc   pm_elf_bad
    cmpl $KSTAGE, %eax              # не заходим на staging-область
    ja   pm_elf_bad
    movl 32(%ebx), %eax             # p_filesz <= p_memsz
    cmpl %ecx, %eax
    ja   pm_elf_bad
    movl 8(%ebx), %eax              # p_offset + p_filesz <= размер файла
    addl 32(%ebx), %eax
    jc   pm_elf_bad
    cmpl file_bytes, %eax
    ja   pm_elf_bad
    pushl %esi
    addl 8(%ebx), %esi              # источник: staging + p_offset
    movl 32(%ebx), %ecx
    rep  movsb                      # копируем filesz байт в p_paddr
    movl 40(%ebx), %ecx
    subl 32(%ebx), %ecx
    xorl %eax, %eax
    rep  stosb                      # обнуляем хвост (.bss)
    popl %esi
pm_ph_next:
    addl %edx, %ebx
    decl %ebp
    jnz  pm_ph_loop

    /* ---- Multiboot2 information */
    movl $MB2_INFO, %edi
    addl $8, %edi                   # [0]=total_size, [4]=reserved - заполним в конце

    movl $1, %eax                   # tag 1: командная строка
    movl sel_rec, %esi
    addl $REC_CMDLINE, %esi
    call add_str_tag
    movl $2, %eax                   # tag 2: имя загрузчика
    movl $str_loader, %esi
    call add_str_tag

    movl $5, (%edi)                 # tag 5: BIOS boot device
    movl $20, 4(%edi)
    movzbl boot_drive, %eax
    movl %eax, 8(%edi)
    movl $0xFFFFFFFF, 12(%edi)
    movl $0xFFFFFFFF, 16(%edi)
    addl $24, %edi

    movl %edi, %ebx                 # tag 6: карта памяти (записи по 24 байта)
    movl $6, (%edi)
    movl $24, 8(%edi)
    movl $0, 12(%edi)
    addl $16, %edi
    movzwl e820_count, %ecx
    movl $E820_BUF, %esi
pm_mm_loop:
    testl %ecx, %ecx
    jz   pm_mm_done
    movsl                           # base (8 байт)
    movsl
    movsl                           # length (8 байт)
    movsl
    movsl                           # type
    addl $4, %esi                   # пропускаем ACPI-атрибуты
    movl $0, (%edi)                 # reserved
    addl $4, %edi
    decl %ecx
    jmp  pm_mm_loop
pm_mm_done:
    movl %edi, %eax
    subl %ebx, %eax
    movl %eax, 4(%ebx)              # размер тега
    addl $7, %edi
    andl $0xFFFFFFF8, %edi

    movl $0, (%edi)                 # tag 0: конец
    movl $8, 4(%edi)
    addl $8, %edi
    movl %edi, %eax
    subl $MB2_INFO, %eax
    movl %eax, MB2_INFO             # total_size
    movl $0, MB2_INFO+4

    movl $0x36D76289, %eax          # Multiboot2 bootloader magic
    movl $MB2_INFO, %ebx
    movl entry_point, %ecx
    jmp  *%ecx

add_str_tag:                        # EAX=тип, ESI=строка, EDI=куда; EDI -> следующий тег
    pushl %ebx
    movl %eax, (%edi)
    movl %edi, %ebx
    addl $8, %edi
1:  lodsb
    stosb
    testb %al, %al
    jnz  1b
    movl %edi, %eax
    subl %ebx, %eax
    movl %eax, 4(%ebx)              # размер = 8 + len + 1
    addl $7, %edi
    andl $0xFFFFFFF8, %edi
    popl %ebx
    ret

pm_a20_fail:
    movl $msg_a20, %esi
    jmp  pm_die
pm_elf_bad:
    movl $msg_elf, %esi
pm_die:                             # ESI = сообщение, красная строка внизу экрана
    movl $(0xB8000 + 22*160), %edi
    movb $0x4F, %ah
1:  lodsb
    testb %al, %al
    jz   2f
    movw %ax, (%edi)
    addl $2, %edi
    jmp  1b
2:  cli
    hlt
    jmp  2b

/* ================================================================ данные */
.align 8
gdt:
    .quad 0
    .quad 0x00CF9A000000FFFF        # 0x08: код 32 бита, flat
    .quad 0x00CF92000000FFFF        # 0x10: данные, flat
gdt_end:
gdt_ptr:
    .word gdt_end - gdt - 1
    .long gdt

.align 4
dap:
    .byte 0x10, 0
dap_count:  .word 0
dap_off:    .word 0
dap_seg:    .word 0
dap_lba:    .long 0, 0

.align 8
gdt87:                              # таблица для INT 15h/87h
    .quad 0, 0
    .word 0xFFFF                    # источник: bounce 0x010000
    .byte 0x00, 0x00, 0x01
    .byte 0x93, 0x00, 0x00
    .word 0xFFFF                    # приёмник: k_dest (патчится)
gdt87_dst_lo:  .word 0
gdt87_dst_mid: .byte 0
    .byte 0x93, 0x00, 0x00
    .quad 0, 0

boot_drive:   .byte 0
scan_drive:   .byte 0
cur_drive:    .byte 0
sel_drive:    .byte 0
.align 4
sel_rec:      .long 0
file_bytes:   .long 0
entry_point:  .long 0
k_lba:        .long 0
k_left:       .long 0
k_dest:       .long 0
e820_count:   .word 0
menu_count:   .word 0
default_idx:  .word 0
sel_idx:      .word 0
start_ticks:  .word 0
chunk:        .word 0

str_loader:   .asciz "Kolon Kiwi 0.1"
msg_banner:   .asciz "Kolon Kiwi 0.1\r\n"
msg_menu:     .asciz "\r\nSelect OS:\r\n"
msg_lb:       .asciz "  ["
msg_rb:       .asciz "] "
msg_other:    .asciz " on another Disk (on /dev/sd"
msg_close:    .asciz ")"
msg_crlf:     .asciz "\r\n"
msg_prompt:   .asciz "\r\nPress 1-9 to choose, Enter = default (5 s)... "
msg_booting:  .asciz "Booting "
msg_noos:     .asciz "No Kolon Kiwi OS found on any disk\r\n"
msg_e820:     .asciz "E820 memory map failed\r\n"
msg_badsize:  .asciz "Bad kernel size\r\n"
msg_diskerr:  .asciz "Disk read error\r\n"
msg_copyerr:  .asciz "BIOS copy (INT 15h/87h) error\r\n"
msg_a20:      .asciz "A20 line is disabled"
msg_elf:      .asciz "Kernel is not a valid x86_64 ELF64 image"
