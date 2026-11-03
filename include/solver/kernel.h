#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include "io.h"

#define SOLVER_NAME    "SolverUNIX"
#define SOLVER_VERSION "0.3.0"
#define SOLVER_ARCH    "x86_64"

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t  i32;
typedef int64_t  i64;

/* Состояние регистров, которое кладёт isr_common (arch/x86_64/isr.S).
 * Порядок = обратный порядку push; последние 5 полей кладёт сам CPU. */
struct regs {
    u64 r15, r14, r13, r12, r11, r10, r9, r8;
    u64 rbp, rdi, rsi, rdx, rcx, rbx, rax;
    u64 int_no, err_code;
    u64 rip, cs, rflags, rsp, ss;
};

typedef void (*irq_handler_t)(struct regs *);

/* arch/x86_64 */
void gdt_init(void);
void tss_set_rsp0(u64 rsp0);
void idt_init(void);
void irq_install_handler(int irq, irq_handler_t h);

/* drivers */
void vga_init(void);
void vga_putc(char c);
void vga_clear(void);
void vga_set_color(u8 fg, u8 bg);
void pit_init(u32 hz);
u32  pit_ticks(void);
u32  pit_hz(void);
void pit_sleep_ms(u32 ms);
void serial_init(void);
void serial_putc(char c);
void kbd_init(void);
char kbd_getchar(void);          /* блокирующее чтение */

/* kernel */
void kputc(char c);
void kputs(const char *s);
void kprintf(const char *fmt, ...);   /* %c %s %d %u %x %p, модификатор l для 64 бит */
void kvprintf(const char *fmt, va_list ap);
void panic(const char *fmt, ...) __attribute__((noreturn));
void heap_init(void *start, size_t size);
void *kmalloc(size_t n);
void kfree(void *p);
size_t heap_free_bytes(void);
u64  syscall_dispatch(struct regs *r);
void solsh_run(void);

/* lib/solibc_k.c  (ядерная часть solibc) */
void  *memcpy(void *d, const void *s, size_t n);
void  *memmove(void *d, const void *s, size_t n);
void  *memset(void *d, int c, size_t n);
int    memcmp(const void *a, const void *b, size_t n);
size_t strlen(const char *s);
int    strcmp(const char *a, const char *b);
int    strncmp(const char *a, const char *b, size_t n);
char  *strcpy(char *d, const char *s);

extern char kernel_end[];
