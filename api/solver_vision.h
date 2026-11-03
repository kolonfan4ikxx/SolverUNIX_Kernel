#ifndef SOLVER_VISION_H
#define SOLVER_VISION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SOLVER_VISION_VERSION "0.1.0"

typedef enum {
    SOLVER_VISION_UNKNOWN = 0,
    SOLVER_VISION_NVIDIA,
    SOLVER_VISION_AMD,
    SOLVER_VISION_INTEL
} solver_vision_vendor_t;

typedef struct {
    solver_vision_vendor_t vendor;
    uint32_t device_id;
    uint32_t vendor_id;
    const char* name;
} solver_vision_device_t;

int solver_vision_init(void);
void solver_vision_shutdown(void);

int solver_vision_detect(
    solver_vision_device_t* device
);

int solver_vision_get_device_count(void);

int solver_vision_enable_acceleration(void);

int solver_vision_is_available(void);

#ifdef __cplusplus
}
#endif

#endif