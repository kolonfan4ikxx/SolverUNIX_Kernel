#ifndef SOLERROR_H
#define SOLERROR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SOL_SUCCESS = 0,
    SOL_ERROR_UNKNOWN = -1,
    SOL_ERROR_INVALID_ARGUMENT = -2,
    SOL_ERROR_NOT_FOUND = -3,
    SOL_ERROR_NO_MEMORY = -4,
    SOL_ERROR_PERMISSION = -5,
    SOL_ERROR_NOT_SUPPORTED = -6,
    SOL_ERROR_IO = -7,
    SOL_ERROR_TIMEOUT = -8,
    SOL_ERROR_BUSY = -9,
    SOL_ERROR_NOT_INITIALIZED = -10
} sol_error_t;

const char* sol_error_string(sol_error_t error);

#ifdef __cplusplus
}
#endif

#endif