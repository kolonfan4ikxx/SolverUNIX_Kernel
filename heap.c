#include <solver/kernel.h>

/* Простой аллокатор: связный список блоков, first-fit, слияние при free.
 * Заголовок 32 байта и выравнивание 16 -> kmalloc отдаёт 16-байтные адреса. */
struct blk {
    size_t size;
    int free;
    struct blk *next;
} __attribute__((aligned(16)));

static struct blk *head;

void heap_init(void *start, size_t size)
{
    head = (struct blk *)start;
    head->size = size - sizeof(struct blk);
    head->free = 1;
    head->next = NULL;
}

void *kmalloc(size_t n)
{
    n = (n + 15) & ~(size_t)15;
    for (struct blk *b = head; b; b = b->next) {
        if (!b->free || b->size < n) continue;
        if (b->size >= n + sizeof(struct blk) + 16) {     /* разрезаем */
            struct blk *s = (struct blk *)((u8 *)(b + 1) + n);
            s->size = b->size - n - sizeof(struct blk);
            s->free = 1;
            s->next = b->next;
            b->next = s;
            b->size = n;
        }
        b->free = 0;
        return b + 1;
    }
    return NULL;
}

void kfree(void *p)
{
    if (!p) return;
    ((struct blk *)p - 1)->free = 1;
    for (struct blk *b = head; b; b = b->next)
        while (b->free && b->next && b->next->free) {
            b->size += sizeof(struct blk) + b->next->size;
            b->next = b->next->next;
        }
}

size_t heap_free_bytes(void)
{
    size_t t = 0;
    for (struct blk *b = head; b; b = b->next) if (b->free) t += b->size;
    return t;
}
