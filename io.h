#pragma once
#include <stdint.h>

static inline void outb(uint16_t port, uint8_t v)
{
    __asm__ volatile("outb %0, %1" : : "a"(v), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t r;
    __asm__ volatile("inb %1, %0" : "=a"(r) : "Nd"(port));
    return r;
}

static inline void io_wait(void) { outb(0x80, 0); }
static inline void cli(void)     { __asm__ volatile("cli"); }
static inline void sti(void)     { __asm__ volatile("sti"); }
static inline void hlt(void)     { __asm__ volatile("hlt"); }
