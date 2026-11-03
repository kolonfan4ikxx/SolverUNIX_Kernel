#ifndef SOLIBC_H
#define SOLIBC_H

#include <stdint.h>
#include <stddef.h>
#include "solerror.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SOLIBC_VERSION_MAJOR 0
#define SOLIBC_VERSION_MINOR 1
#define SOLIBC_VERSION_PATCH 0

#define SOLIBC_VERSION "0.1.0"

int sol_init(void);
void sol_shutdown(void);

const char* sol_get_version(void);
const char* sol_get_system_name(void);

uint64_t sol_get_uptime(void);

int sol_get_last_error(void);

#ifdef __cplusplus
}
#endif

#endif