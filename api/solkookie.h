#ifndef SOLKOOKIE_H
#define SOLKOOKIE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SOLKOOKIE_VERSION "0.1.0"

typedef struct {
    char session_id[64];
    char domain[256];
    uint64_t expires_at;
    uint8_t valid;
} sol_kookie_t;

int sol_kookie_init(void);

int sol_kookie_create(
    const char* domain,
    sol_kookie_t* cookie
);

int sol_kookie_validate(
    const sol_kookie_t* cookie
);

int sol_kookie_revoke(
    const char* session_id
);

void sol_kookie_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif