#ifndef __TTF_p8_SCREEN_H__
#define __TTF_p8_SCREEN_H__

#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif
#include <stdint.h>
#include <stddef.h>
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "freertos/semphr.h"
#include "dgx_screen.h"

void dgx_p8_sync_send_cmd(dgx_screen_t *scr, const uint8_t cmd);
void dgx_p8_sync_send_data(dgx_screen_t *scr, const uint8_t *data, int len);
void dgx_p8_set_area(dgx_screen_t *scr, uint16_t caset_lo, uint16_t caset_hi, uint16_t raset_lo, uint16_t raset_hi);
void dgx_p8_update_screen(dgx_screen_t *scr, int16_t x1, int16_t y1, int16_t x2, int16_t y2);
esp_err_t dgx_p8_init(dgx_screen_t *scr, gpio_num_t lcd_d0, gpio_num_t lcd_d1, gpio_num_t lcd_d2, gpio_num_t lcd_d3,
        gpio_num_t lcd_d4, gpio_num_t lcd_d5, gpio_num_t lcd_d6, gpio_num_t lcd_d7, gpio_num_t lcd_wr,
        gpio_num_t lcd_rd, gpio_num_t cs, gpio_num_t dc);
void dgx_p8_crset(dgx_screen_t *scr, uint16_t caset_lo, uint16_t caset_hi, uint16_t raset_lo, uint16_t raset_hi);
void dgx_p8_write_ram(dgx_screen_t *scr);
void dgx_p8_write_data(dgx_screen_t *scr, uint8_t *data, uint32_t lenbits);
void dgx_p8_write_value(dgx_screen_t *scr, uint32_t value);
void dgx_p8_wait_data(dgx_screen_t *scr);
void dgx_p8_draw_pixel(dgx_screen_t *scr, int16_t x, int16_t y, uint32_t color);
uint32_t dgx_p8_read_pixel(dgx_screen_t *scr, int16_t x, int16_t y);
void dgx_p8_fill_line_buffer(dgx_screen_t *scr, int16_t w, uint32_t color, uint32_t bg, uint32_t mask,
        uint8_t mask_bits);
void dgx_p8_fill_hline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, uint32_t color, uint32_t bg, uint32_t mask,
        uint8_t mask_bits);
void dgx_p8_fill_vline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t h, uint32_t color, uint32_t bg, uint32_t mask,
        uint8_t mask_bits);
void dgx_p8_fill_rectangle(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color);
void dgx_p8_fast_vline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t h, uint32_t color);
void dgx_p8_fast_hline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, uint32_t color);

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on

#endif

#endif /* __TTF_p8_SCREEN_H__ */
