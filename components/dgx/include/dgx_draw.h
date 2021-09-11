/*
 * MIT License
 *
 * Copyright (c) 2021 Anton Petrusevich
 *
 */

#ifndef __TTF_DRAW_H__
#define __TTF_DRAW_H__

#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

#include "dgx_screen.h"

void dgx_draw_line(dgx_screen_t *scr, int16_t x, int16_t y, int16_t x2, int16_t y2, uint32_t color);
void dgx_draw_line_float(dgx_screen_t *scr, float xo, float yo, float x2o, float y2o, uint32_t color);
uint32_t dgx_draw_line_mask(dgx_screen_t *scr, int16_t x, int16_t y, int16_t x2, int16_t y2, uint32_t color, uint32_t bg, uint32_t mask, uint8_t mask_bits);
void dgx_draw_circle(dgx_screen_t *scr, int16_t x, int16_t y, int16_t r, uint32_t color);
void dgx_draw_circle_solid(dgx_screen_t *scr, int16_t x, int16_t y, int16_t r, uint32_t color);
uint32_t dgx_draw_circle_mask(dgx_screen_t *scr, int16_t x, int16_t y, int16_t r, uint32_t color, uint32_t bg, uint32_t mask, uint8_t mask_bits);
void dgx_fill_rectangle(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color);
void dgx_draw_triangle_solid(dgx_screen_t *scr, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint32_t color);
void dgx_draw_line_thick(dgx_screen_t *scr, int16_t x, int16_t y, int16_t x2, int16_t y2, int16_t width, uint32_t color);
#ifdef __cplusplus
// @formatter:off
}
// @formatter:on

#endif

#endif /* __TTF_SCREEN_H__ */
