#include <solver/kernel.h>

#define W 80
#define H 25

static volatile u16 *const VGA = (volatile u16 *)0xB8000;
static int cx, cy;
static u8 color = 0x07;

static void cursor(void)
{
    u16 pos = cy * W + cx;
    outb(0x3D4, 14); outb(0x3D5, pos >> 8);
    outb(0x3D4, 15); outb(0x3D5, pos & 0xFF);
}

static void scroll(void)
{
    for (int i = 0; i < W * (H - 1); i++) VGA[i] = VGA[i + W];
    for (int i = 0; i < W; i++) VGA[W * (H - 1) + i] = (color << 8) | ' ';
    cy = H - 1;
}

void vga_set_color(u8 fg, u8 bg) { color = (bg << 4) | (fg & 0x0F); }

void vga_clear(void)
{
    for (int i = 0; i < W * H; i++) VGA[i] = (color << 8) | ' ';
    cx = cy = 0;
    cursor();
}

void vga_init(void) { vga_clear(); }

void vga_putc(char c)
{
    if (c == '\n')      { cx = 0; cy++; }
    else if (c == '\r') { cx = 0; }
    else if (c == '\t') { cx = (cx + 8) & ~7; }
    else if (c == '\b') { if (cx > 0) { cx--; VGA[cy * W + cx] = (color << 8) | ' '; } }
    else                { VGA[cy * W + cx] = (color << 8) | (u8)c; cx++; }

    if (cx >= W) { cx = 0; cy++; }
    if (cy >= H) scroll();
    cursor();
}
