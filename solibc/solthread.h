#ifndef SOLTHREAD_H
#define SOLTHREAD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t sol_thread_t;
typedef void* (*sol_thread_func_t)(void* argument);

typedef struct {
    volatile uint32_t value;
} sol_mutex_t;

int sol_thread_create(
    sol_thread_t* thread,
    sol_thread_func_t function,
    void* argument
);

int sol_thread_join(sol_thread_t thread);

int sol_mutex_init(sol_mutex_t* mutex);
int sol_mutex_lock(sol_mutex_t* mutex);
int sol_mutex_unlock(sol_mutex_t* mutex);
int sol_mutex_destroy(sol_mutex_t* mutex);

#ifdef __cplusplus
}
#endif

#endif