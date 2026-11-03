#include <solver/kernel.h>

static const char map[58] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b','\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'','`', 0,'\\',
    'z','x','c','v','b','n','m',',','.','/', 0,'*', 0,' '
};
static const char smap[58] = {
    0, 27, '!','@','#','$','%','^','&','*','(',')','_','+','\b','\t',
    'Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0,
    'A','S','D','F','G','H','J','K','L',':','"','~', 0,'|',
    'Z','X','C','V','B','N','M','<','>','?', 0,'*', 0,' '
};

#define BUF 256
static volatile char buf[BUF];
static volatile u32 head, tail;
static int shift, caps;

static void push(char c)
{
    u32 n = (head + 1) % BUF;
    if (n != tail) { buf[head] = c; head = n; }
}

static void kbd_irq(struct regs *r)
{
    (void)r;
    u8 sc = inb(0x60);

    if (sc == 0x2A || sc == 0x36) { shift = 1; return; }
    if (sc == 0xAA || sc == 0xB6) { shift = 0; return; }
    if (sc == 0x3A)               { caps ^= 1; return; }
    if (sc & 0x80 || sc >= sizeof(map)) return;

    char c = shift ? smap[sc] : map[sc];
    if (caps && !shift && c >= 'a' && c <= 'z') c -= 32;
    else if (caps && shift && c >= 'A' && c <= 'Z') c += 32;
    if (c) push(c);
}

void kbd_init(void) { irq_install_handler(1, kbd_irq); }

char kbd_getchar(void)
{
    while (head == tail) __asm__ volatile("sti; hlt");
    char c = buf[tail];
    tail = (tail + 1) % BUF;
    return c;
}
