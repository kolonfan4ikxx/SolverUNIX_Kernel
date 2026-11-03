#ifndef SOLWEB_H
#define SOLWEB_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SOLV2_VERSION "0.1.0"

typedef enum {
    SOL_WEB_GET,
    SOL_WEB_POST,
    SOL_WEB_PUT,
    SOL_WEB_DELETE
} sol_web_method_t;

typedef struct {
    sol_web_method_t method;
    const char* url;
    const char* user_agent;
    uint32_t timeout_ms;
} sol_web_request_t;

typedef struct {
    int status_code;
    void* data;
    size_t data_size;
} sol_web_response_t;

int sol_web_init(void);
void sol_web_shutdown(void);

int sol_web_request(
    const sol_web_request_t* request,
    sol_web_response_t* response
);

void sol_web_free_response(
    sol_web_response_t* response
);

const char* sol_web_get_version(void);

#ifdef __cplusplus
}
#endif

#endif