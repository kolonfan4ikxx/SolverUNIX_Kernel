#include <solver/multiboot2.h>

const struct mb2_info *g_boot_info;

const struct mb2_tag *mb2_find_tag(const struct mb2_info *info, u32 type)
{
    const u8 *base = (const u8 *)info;
    const u8 *end  = base + info->total_size;
    const u8 *p    = base + 8;

    while (p + sizeof(struct mb2_tag) <= end) {
        const struct mb2_tag *t = (const struct mb2_tag *)p;
        if (t->type == MB2_TAG_END) return NULL;
        if (t->type == type)        return t;
        if (t->size < 8)            return NULL;
        p += (t->size + 7) & ~7u;
    }
    return NULL;
}
