#ifndef SOLMEMORY_H
#define SOLMEMORY_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* sol_malloc(size_t size);
void* sol_calloc(size_t count, size_t size);
void* sol_realloc(void* ptr, size_t size);
void sol_free(void* ptr);

void* sol_memcpy(void* destination,
                 const void* source,
                 size_t size);

void* sol_memset(void* destination,
                 int value,
                 size_t size);

int sol_memcmp(const void* a,
               const void* b,
               size_t size);

#ifdef __cplusplus
}
#endif

#endif