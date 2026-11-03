#include <solver/kernel.h>
#include <solver/syscall.h>

u64 syscall_dispatch(struct regs *r)
{
    switch (r->rax) {
    case SYS_WRITE: {
        const char *p = (const char *)r->rsi;
        u64 n = r->rdx;
        if (r->rdi != 1 && r->rdi != 2) return (u64)-1;   /* только stdout/stderr */
        for (u64 i = 0; i < n; i++) kputc(p[i]);
        return n;
    }
    case SYS_TICKS:   return pit_ticks();
    case SYS_GETCHAR: return (u64)kbd_getchar();
    case SYS_UNAME:   return (u64)(SOLVER_NAME " " SOLVER_VERSION " " SOLVER_ARCH);
    default:          return (u64)-1;                     /* ENOSYS */
    }
}
