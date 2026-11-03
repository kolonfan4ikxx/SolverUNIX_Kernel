#ifndef SOLGRAPHICS_H
#define SOLGRAPHICS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SOL_GFX_UNKNOWN = 0,
    SOL_GFX_FRAMEBUFFER,
    SOL_GFX_VESA,
    SOL_GFX_GENERIC,
    SOL_GFX_SOLVER_VISION
} sol_gfx_backend_t;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bits_per_pixel;
} sol_gfx_mode_t;

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} sol_gfx_rect_t;

int sol_graphics_init(void);
void sol_graphics_shutdown(void);

int sol_graphics_set_mode(
    const sol_gfx_mode_t* mode
);

int sol_graphics_get_mode(
    sol_gfx_mode_t* mode
);

int sol_graphics_get_backend(
    sol_gfx_backend_t* backend
);

int sol_graphics_clear(uint32_t color);

int sol_graphics_present(void);

#ifdef __cplusplus
}
#endif

#endif