#include <solver/kernel.h>

void kputc(char c) { vga_putc(c); serial_putc(c); }
void kputs(const char *s) { while (*s) kputc(*s++); }

static void put_num(u64 v, u32 base, int width, char pad)
{
    char tmp[64]; int n = 0;
    do { tmp[n++] = "0123456789abcdef"[v % base]; v /= base; } while (v);
    for (int i = n; i < width; i++) kputc(pad);
    while (n) kputc(tmp[--n]);
}

void kvprintf(const char *f, va_list ap)
{
    for (; *f; f++) {
        if (*f != '%') { kputc(*f); continue; }
        f++;
        int left = 0, lng = 0;
        char pad = ' '; int width = 0;
        if (*f == '-') { left = 1; f++; }
        if (*f == '0') { pad = '0'; f++; }
        while (*f >= '0' && *f <= '9') width = width * 10 + (*f++ - '0');
        if (*f == 'l') { lng = 1; f++; }

        switch (*f) {
        case 'c': kputc((char)va_arg(ap, int)); break;
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            int len = (int)strlen(s);
            if (!left) for (int i = len; i < width; i++) kputc(' ');
            kputs(s);
            if (left)  for (int i = len; i < width; i++) kputc(' ');
            break;
        }
        case 'd': {
            i64 v = lng ? va_arg(ap, i64) : (i64)va_arg(ap, i32);
            if (v < 0) { kputc('-'); put_num((u64)-v, 10, width, pad); }
            else       { put_num((u64)v, 10, width, pad); }
            break;
        }
        case 'u': put_num(lng ? va_arg(ap, u64) : va_arg(ap, u32), 10, width, pad); break;
        case 'x': put_num(lng ? va_arg(ap, u64) : va_arg(ap, u32), 16, width, pad); break;
        case 'p': kputs("0x"); put_num((u64)va_arg(ap, void *), 16, 16, '0'); break;
        case '%': kputc('%'); break;
        case 0:   return;
        default:  kputc('%'); kputc(*f); break;
        }
    }
}

void kprintf(const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt); kvprintf(fmt, ap); va_end(ap);
}

void panic(const char *fmt, ...)
{
    cli();
    vga_set_color(15, 4);
    kputs("\n*** SolverUNIX KERNEL PANIC ***\n");
    va_list ap; va_start(ap, fmt); kvprintf(fmt, ap); va_end(ap);
    kputs("\nSystem halted.");
    for (;;) hlt();
}
