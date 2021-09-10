#ifndef __TTF_v_SCREEN_H__
#define __TTF_v_SCREEN_H__

#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif
#include <stdint.h>
#include <stddef.h>
#include "dgx_screen.h"

esp_err_t dgx_v_init(dgx_screen_t *scr, int16_t width, int16_t height, uint8_t color_bits);
void dgx_v_draw_pixel(dgx_screen_t *scr, int16_t x, int16_t y, uint32_t color);
uint32_t dgx_v_read_pixel(dgx_screen_t *scr, int16_t x, int16_t y);
void dgx_v_fill_hline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, uint32_t color, uint32_t bg, uint32_t mask, uint8_t mask_bits);
void dgx_v_fill_vline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t h, uint32_t color, uint32_t bg, uint32_t mask, uint8_t mask_bits);
void dgx_v_fill_rectangle(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color);
void dgx_v_fast_vline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t h, uint32_t color);
void dgx_v_fast_hline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, uint32_t color);
void dgx_v_set_area(dgx_screen_t *scr, uint16_t caset_lo, uint16_t caset_hi, uint16_t raset_lo, uint16_t raset_hi);
void dgx_v_wait_data(dgx_screen_t *scr);
void dgx_v_write_data(dgx_screen_t *scr, uint8_t *data, uint32_t lenbits);
void dgx_v_write_value(dgx_screen_t *scr, uint32_t value);

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on

#endif

#endif /* __TTF_v_SCREEN_H__ */
