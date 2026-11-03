#include <solver/kernel.h>

/* COM1, 115200 8N1. В QEMU: -serial stdio */
#define COM1 0x3F8

void serial_init(void)
{
    outb(COM1 + 1, 0x00);   /* без прерываний */
    outb(COM1 + 3, 0x80);   /* DLAB */
    outb(COM1 + 0, 0x01);   /* делитель 1 -> 115200 */
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);   /* 8N1 */
    outb(COM1 + 2, 0xC7);   /* FIFO */
    outb(COM1 + 4, 0x0B);
}

static void raw(char c)
{
    for (int i = 0; i < 100000 && !(inb(COM1 + 5) & 0x20); i++) ;
    outb(COM1, c);
}

void serial_putc(char c)
{
    if (c == '\n') raw('\r');
    raw(c);
}
