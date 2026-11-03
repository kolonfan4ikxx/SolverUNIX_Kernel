#pragma once
#include <solver/kernel.h>

#define MB2_BOOTLOADER_MAGIC 0x36D76289

enum {
    MB2_TAG_END        = 0,
    MB2_TAG_CMDLINE    = 1,
    MB2_TAG_BOOTLOADER = 2,
    MB2_TAG_MODULE     = 3,
    MB2_TAG_MEMINFO    = 4,
    MB2_TAG_BOOTDEV    = 5,
    MB2_TAG_MMAP       = 6,
};

struct mb2_info { u32 total_size; u32 reserved; };
struct mb2_tag  { u32 type; u32 size; };
struct mb2_str_tag { u32 type; u32 size; char string[]; };

struct mb2_mmap_entry { u64 addr; u64 len; u32 type; u32 reserved; };
struct mb2_tag_mmap {
    u32 type; u32 size; u32 entry_size; u32 entry_version;
    struct mb2_mmap_entry entries[];
};

const struct mb2_tag *mb2_find_tag(const struct mb2_info *info, u32 type);
extern const struct mb2_info *g_boot_info;
