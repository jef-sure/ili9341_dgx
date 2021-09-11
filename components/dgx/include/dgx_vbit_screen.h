/*
 * MIT License
 *
 * Copyright (c) 2021 Anton Petrusevich
 *
 */

#ifndef __TTF_VBIT_SCREEN_H__
#define __TTF_VBIT_SCREEN_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif
#include "dgx_screen.h"

esp_err_t dgx_vbit_init(dgx_screen_t *scr, int16_t width, int16_t height);
void dgx_vbit_draw_pixel(dgx_screen_t *scr, int16_t x, int16_t y, uint32_t color);
uint32_t dgx_vbit_read_pixel(dgx_screen_t *scr, int16_t x, int16_t y);
void dgx_vbit_fill_hline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, uint32_t color, uint32_t bg, uint32_t mask, uint8_t mask_bits);
void dgx_vbit_fill_vline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t h, uint32_t color, uint32_t bg, uint32_t mask, uint8_t mask_bits);
void dgx_vbit_fill_rectangle(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color);
void dgx_vbit_fast_vline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t h, uint32_t color);
void dgx_vbit_fast_hline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, uint32_t color);
void dgx_vbit_set_area(dgx_screen_t *scr, uint16_t caset_lo, uint16_t caset_hi, uint16_t raset_lo, uint16_t raset_hi);
void dgx_vbit_wait_data(dgx_screen_t *scr);
void dgx_vbit_write_data(dgx_screen_t *scr, uint8_t *data, uint32_t lenbits);
void dgx_vbit_write_value(dgx_screen_t *scr, uint32_t value);
void dgx_vbit_to_screen(dgx_screen_t *scr, dgx_screen_t *vbscr, int16_t x, int16_t y, uint32_t color, uint32_t bg_color, dgx_orientation_t xdir, dgx_orientation_t ydir, bool swap_xy);
#ifdef __cplusplus
// @formatter:off
}
// @formatter:on

#endif

#endif /* __TTF_VBIT_SCREEN_H__ */
