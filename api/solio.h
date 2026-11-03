#ifndef SOLIO_H
#define SOLIO_H

#include <stddef.h>
#include <stdint.h>
#include "solerror.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t sol_io_handle_t;

typedef enum {
    SOL_IO_READ = 1,
    SOL_IO_WRITE = 2,
    SOL_IO_READ_WRITE = 3
} sol_io_mode_t;

typedef enum {
    SOL_DEVICE_UNKNOWN = 0,
    SOL_DEVICE_STORAGE,
    SOL_DEVICE_KEYBOARD,
    SOL_DEVICE_MOUSE,
    SOL_DEVICE_DISPLAY,
    SOL_DEVICE_SERIAL,
    SOL_DEVICE_NETWORK
} sol_device_type_t;

int sol_io_init(void);

sol_io_handle_t sol_io_open(
    const char* device,
    sol_io_mode_t mode
);

int sol_io_close(sol_io_handle_t handle);

int64_t sol_io_read(
    sol_io_handle_t handle,
    void* buffer,
    size_t size
);

int64_t sol_io_write(
    sol_io_handle_t handle,
    const void* buffer,
    size_t size
);

int sol_io_flush(sol_io_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif