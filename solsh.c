#include <solver/kernel.h>
#include <solver/syscall.h>
#include <solver/multiboot2.h>

/* Временный встроенный solsh. Когда появится userspace и файловая система,
 * настоящий solsh переедет в отдельный исполняемый файл, а это уйдёт. */

#define MAXLINE 128
#define MAXARGS 16

static int readline(char *buf, int max)
{
    int n = 0;
    for (;;) {
        char c = kbd_getchar();
        if (c == '\n') { kputc('\n'); buf[n] = 0; return n; }
        if (c == '\b') { if (n > 0) { n--; kputc('\b'); } continue; }
        if (c >= 32 && c < 127 && n < max - 1) { buf[n++] = c; kputc(c); }
    }
}

static int tokenize(char *line, char **argv)
{
    int argc = 0;
    while (*line && argc < MAXARGS) {
        while (*line == ' ') *line++ = 0;
        if (!*line) break;
        argv[argc++] = line;
        while (*line && *line != ' ') line++;
    }
    return argc;
}

static void cmd_help(int c, char **v);

static void cmd_echo(int c, char **v)
{
    for (int i = 1; i < c; i++) kprintf("%s%c", v[i], i + 1 < c ? ' ' : '\n');
    if (c == 1) kputc('\n');
}
static void cmd_clear(int c, char **v)  { (void)c; (void)v; vga_clear(); }
static void cmd_uname(int c, char **v)  { (void)c; (void)v; kprintf("%s %s %s\n", SOLVER_NAME, SOLVER_VERSION, SOLVER_ARCH); }
static void cmd_uptime(int c, char **v)
{
    (void)c; (void)v;
    u32 s = pit_ticks() / pit_hz();
    kprintf("up %u:%02u:%02u\n", s / 3600, (s / 60) % 60, s % 60);
}
static void cmd_mem(int c, char **v)
{
    (void)c; (void)v;
    kprintf("heap free: %u KiB\n", (u32)(heap_free_bytes() / 1024));
}
static void cmd_sys(int c, char **v)
{
    (void)c; (void)v;
    const char msg[] = "hello from int 0x80\n";
    syscall3(SYS_WRITE, 1, (unsigned long)msg, sizeof(msg) - 1);
}
static void cmd_alloc(int c, char **v)
{
    (void)c; (void)v;
    void *a = kmalloc(1000), *b = kmalloc(2000);
    kprintf("a=%p b=%p\n", a, b);
    kfree(a); kfree(b);
    kprintf("after free: %u KiB free\n", (u32)(heap_free_bytes() / 1024));
}
static void cmd_bootinfo(int c, char **v)
{
    (void)c; (void)v;
    const struct mb2_str_tag *t = (const void *)mb2_find_tag(g_boot_info, MB2_TAG_BOOTLOADER);
    kprintf("bootloader: %s\n", t ? t->string : "?");
    t = (const void *)mb2_find_tag(g_boot_info, MB2_TAG_CMDLINE);
    kprintf("cmdline   : %s\n", t ? t->string : "");
}
static void cmd_cpu(int c, char **v)
{
    (void)c; (void)v;
    u32 a, b, cc, d;
    char vendor[13];
    __asm__ volatile("cpuid" : "=a"(a), "=b"(b), "=c"(cc), "=d"(d) : "a"(0));
    memcpy(vendor, &b, 4); memcpy(vendor + 4, &d, 4); memcpy(vendor + 8, &cc, 4);
    vendor[12] = 0;
    kprintf("cpu vendor: %s, max cpuid leaf: %u\n", vendor, a);
}
static void cmd_reboot(int c, char **v)
{
    (void)c; (void)v;
    while (inb(0x64) & 2) ;
    outb(0x64, 0xFE);
    for (;;) hlt();
}
static void cmd_halt(int c, char **v)
{
    (void)c; (void)v;
    kputs("System halted. It is safe to power off.\n");
    cli();
    for (;;) hlt();
}
static void cmd_panic(int c, char **v) { (void)c; (void)v; panic("manual panic from solsh"); }

struct cmd { const char *name; void (*fn)(int, char **); const char *help; };
static const struct cmd cmds[] = {
    { "help",   cmd_help,   "list commands" },
    { "echo",   cmd_echo,   "print arguments" },
    { "clear",  cmd_clear,  "clear screen" },
    { "uname",  cmd_uname,  "system info" },
    { "uptime", cmd_uptime, "time since boot" },
    { "mem",    cmd_mem,    "free kernel heap" },
    { "alloc",  cmd_alloc,  "test kmalloc/kfree" },
    { "sys",    cmd_sys,    "test int 0x80 syscall" },
    { "bootinfo", cmd_bootinfo, "bootloader name and cmdline" },
    { "cpu",    cmd_cpu,    "CPU vendor (cpuid)" },
    { "reboot", cmd_reboot, "reboot" },
    { "halt",   cmd_halt,   "halt" },
    { "panic",  cmd_panic,  "trigger kernel panic" },
};
#define NCMDS (sizeof(cmds) / sizeof(cmds[0]))

static void cmd_help(int c, char **v)
{
    (void)c; (void)v;
    for (u32 i = 0; i < NCMDS; i++) kprintf("  %-9s %s\n", cmds[i].name, cmds[i].help);
}

void solsh_run(void)
{
    char line[MAXLINE], *argv[MAXARGS];
    for (;;) {
        kputs("solsh# ");
        if (!readline(line, MAXLINE)) continue;
        int argc = tokenize(line, argv);
        if (!argc) continue;
        int found = 0;
        for (u32 i = 0; i < NCMDS; i++)
            if (!strcmp(argv[0], cmds[i].name)) { cmds[i].fn(argc, argv); found = 1; break; }
        if (!found) kprintf("solsh: %s: command not found\n", argv[0]);
    }
}
