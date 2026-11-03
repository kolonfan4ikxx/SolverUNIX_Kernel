#include <solver/kernel.h>
#include <solver/multiboot2.h>

#define HEAP_SIZE      (4ul * 1024 * 1024)
#define IDENTITY_LIMIT (1ul << 30)      /* boot.S отображает первый 1 GiB */

static const char *mmap_type(u32 t)
{
    switch (t) {
    case 1:  return "usable";
    case 3:  return "ACPI reclaimable";
    case 4:  return "ACPI NVS";
    case 5:  return "bad";
    default: return "reserved";
    }
}

void kmain(u32 magic, u32 info_addr)
{
    serial_init();
    vga_init();
    vga_set_color(11, 0);
    kprintf("%s %s (%s)\n", SOLVER_NAME, SOLVER_VERSION, SOLVER_ARCH);
    vga_set_color(7, 0);

    if (magic != MB2_BOOTLOADER_MAGIC)
        panic("not booted via Multiboot2 (magic=%x)", magic);
    g_boot_info = (const struct mb2_info *)(u64)info_addr;

    const struct mb2_str_tag *bl = (const void *)mb2_find_tag(g_boot_info, MB2_TAG_BOOTLOADER);
    kprintf("[ ok ] Multiboot2 via %s, long mode, paging (identity 1 GiB)\n",
            bl ? bl->string : "unknown loader");

    gdt_init();           kprintf("[ ok ] GDT + TSS\n");
    idt_init();           kprintf("[ ok ] IDT + PIC\n");
    pit_init(100);        kprintf("[ ok ] PIT 100 Hz\n");
    kbd_init();           kprintf("[ ok ] keyboard\n");

    const struct mb2_tag_mmap *mm = (const void *)mb2_find_tag(g_boot_info, MB2_TAG_MMAP);
    if (!mm) panic("bootloader provided no memory map");

    u64 heap_start = ((u64)kernel_end + 0xFFF) & ~0xFFFul;
    u64 heap_end   = heap_start + HEAP_SIZE;
    int in_usable  = 0;
    const u8 *e = (const u8 *)mm->entries, *mm_end = (const u8 *)mm + mm->size;
    for (; e + mm->entry_size <= mm_end; e += mm->entry_size) {
        const struct mb2_mmap_entry *me = (const void *)e;
        kprintf("       %p-%p %s\n", (void *)me->addr,
                (void *)(me->addr + me->len), mmap_type(me->type));
        if (me->type == 1 && heap_start >= me->addr && heap_start < me->addr + me->len) {
            if (heap_end > me->addr + me->len) heap_end = me->addr + me->len;
            in_usable = 1;
        }
    }
    if (!in_usable) panic("kernel heap is not inside usable RAM");
    if (heap_end > IDENTITY_LIMIT) heap_end = IDENTITY_LIMIT;
    if (heap_end <= heap_start + 4096) panic("not enough RAM for kernel heap");
    heap_init((void *)heap_start, heap_end - heap_start);
    kprintf("[ ok ] heap %lu KiB at %p\n", (heap_end - heap_start) / 1024, (void *)heap_start);

    sti();
    kprintf("\nWelcome to SolverUNIX. Type 'help'.\n\n");
    solsh_run();
}
