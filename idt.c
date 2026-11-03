#include <solver/kernel.h>

struct idt_entry {
    u16 off_lo;
    u16 sel;
    u8  ist;
    u8  flags;
    u16 off_mid;
    u32 off_hi;
    u32 zero;
} __attribute__((packed));

struct idt_ptr { u16 limit; u64 base; } __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr   ip;
static irq_handler_t    irq_handlers[16];

extern u64 isr_table[48];
extern void isr128(void);
extern void idt_flush(struct idt_ptr *);

static const char *exc_names[32] = {
    "Divide error", "Debug", "NMI", "Breakpoint", "Overflow",
    "Bound range exceeded", "Invalid opcode", "Device not available",
    "Double fault", "Coprocessor overrun", "Invalid TSS",
    "Segment not present", "Stack fault", "General protection fault",
    "Page fault", "Reserved", "x87 FPU error", "Alignment check",
    "Machine check", "SIMD FP exception", "Virtualization",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved", "Security", "Reserved"
};

static void set_gate(int n, u64 handler, u8 ist, u8 flags)
{
    idt[n].off_lo  = handler & 0xFFFF;
    idt[n].sel     = 0x08;
    idt[n].ist     = ist;
    idt[n].flags   = flags;
    idt[n].off_mid = (handler >> 16) & 0xFFFF;
    idt[n].off_hi  = (u32)(handler >> 32);
    idt[n].zero    = 0;
}

static void pic_remap(void)
{
    outb(0x20, 0x11); io_wait();  outb(0xA0, 0x11); io_wait();
    outb(0x21, 0x20); io_wait();  outb(0xA1, 0x28); io_wait();  /* смещения 32/40 */
    outb(0x21, 0x04); io_wait();  outb(0xA1, 0x02); io_wait();
    outb(0x21, 0x01); io_wait();  outb(0xA1, 0x01); io_wait();
    outb(0x21, 0x00);             outb(0xA1, 0x00);
}

void idt_init(void)
{
    pic_remap();
    for (int i = 0; i < 48; i++)
        set_gate(i, isr_table[i], i == 8 ? 1 : 0, 0x8E);   /* #DF на отдельном стеке (IST1) */
    set_gate(0x80, (u64)isr128, 0, 0xEE);                  /* syscall: доступен из ring 3 */
    ip.limit = sizeof(idt) - 1;
    ip.base  = (u64)idt;
    idt_flush(&ip);
}

void irq_install_handler(int irq, irq_handler_t h)
{
    if (irq >= 0 && irq < 16) irq_handlers[irq] = h;
}

void isr_handler(struct regs *r)
{
    u64 n = r->int_no;

    if (n == 0x80) {
        r->rax = syscall_dispatch(r);
        return;
    }
    if (n >= 32 && n < 48) {
        if (irq_handlers[n - 32]) irq_handlers[n - 32](r);
        if (n >= 40) outb(0xA0, 0x20);        /* EOI ведомому PIC */
        outb(0x20, 0x20);                     /* EOI ведущему PIC */
        return;
    }
    if (n == 14) {
        u64 cr2;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
        panic("Page fault at %p\n  err=%lx rip=%p rsp=%p", (void *)cr2,
              r->err_code, (void *)r->rip, (void *)r->rsp);
    }
    panic("CPU exception %lu: %s\n  err=%lx rip=%p cs=%lx rflags=%lx rsp=%p",
          n, n < 32 ? exc_names[n] : "?", r->err_code, (void *)r->rip,
          r->cs, r->rflags, (void *)r->rsp);
}
