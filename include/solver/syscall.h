#pragma once
/* ABI системных вызовов SolverUNIX x86_64 (int 0x80):
 *   rax = номер, rdi/rsi/rdx = аргументы 1..3, результат в rax.
 * (Позже добавим быстрый путь через SYSCALL/SYSRET с той же нумерацией.) */
#define SYS_WRITE   1   /* (fd, buf, len)  -> записано байт */
#define SYS_TICKS   2   /* ()              -> тики таймера   */
#define SYS_GETCHAR 3   /* ()              -> символ         */
#define SYS_UNAME   4   /* ()              -> указатель на строку */

static inline long syscall3(unsigned long n, unsigned long a,
                            unsigned long b, unsigned long c)
{
    long r;
    __asm__ volatile("int $0x80"
                     : "=a"(r)
                     : "a"(n), "D"(a), "S"(b), "d"(c)
                     : "memory");
    return r;
}
