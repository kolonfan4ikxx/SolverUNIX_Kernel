#include <solver/kernel.h>

static volatile u32 ticks;
static u32 freq;

static void pit_irq(struct regs *r) { (void)r; ticks++; }

void pit_init(u32 hz)
{
    freq = hz;
    u32 div = 1193182 / hz;
    outb(0x43, 0x36);
    outb(0x40, div & 0xFF);
    outb(0x40, (div >> 8) & 0xFF);
    irq_install_handler(0, pit_irq);
}

u32 pit_ticks(void) { return ticks; }
u32 pit_hz(void)    { return freq; }

void pit_sleep_ms(u32 ms)
{
    u32 end = ticks + (ms * freq) / 1000 + 1;
    while ((i32)(end - ticks) > 0) hlt();
}
