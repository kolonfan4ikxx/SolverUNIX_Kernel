#include <solver/kernel.h>

/* Селекторы: 0x08 код0, 0x10 данные0, 0x18 данные3, 0x20 код3, 0x28 TSS */
struct gdt_ptr { u16 limit; u64 base; } __attribute__((packed));

struct tss {
    u32 reserved0;
    u64 rsp0, rsp1, rsp2;
    u64 reserved1;
    u64 ist[7];
    u64 reserved2;
    u16 reserved3;
    u16 iopb;
} __attribute__((packed));

static u64 gdt[7] __attribute__((aligned(16)));
static struct gdt_ptr gp;
static struct tss tss;
static u8 ist1_stack[8192] __attribute__((aligned(16)));   /* для double fault */

extern char stack_top[];
extern void gdt_flush(struct gdt_ptr *);
extern void tss_flush(u16);

void tss_set_rsp0(u64 rsp0) { tss.rsp0 = rsp0; }

void gdt_init(void)
{
    u64 base  = (u64)&tss;
    u64 limit = sizeof(tss) - 1;

    tss.rsp0   = (u64)stack_top;
    tss.ist[0] = (u64)(ist1_stack + sizeof(ist1_stack));
    tss.iopb   = sizeof(tss);

    gdt[0] = 0;
    gdt[1] = 0x00209A0000000000ULL;   /* код ring 0   */
    gdt[2] = 0x0000920000000000ULL;   /* данные ring 0 */
    gdt[3] = 0x0000F20000000000ULL;   /* данные ring 3 */
    gdt[4] = 0x0020FA0000000000ULL;   /* код ring 3   */
    gdt[5] = (limit & 0xFFFF)
           | ((base & 0xFFFFFF) << 16)
           | (0x89ULL << 40)                       /* present, 64-bit TSS (available) */
           | (((limit >> 16) & 0xF) << 48)
           | (((base >> 24) & 0xFF) << 56);
    gdt[6] = base >> 32;

    gp.limit = sizeof(gdt) - 1;
    gp.base  = (u64)gdt;
    gdt_flush(&gp);
    tss_flush(0x28);
}
