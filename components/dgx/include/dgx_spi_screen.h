/*
 * MIT License
 *
 * Copyright (c) 2021 Anton Petrusevich
 *
 */

#ifndef __TTF_SPI_SCREEN_H__
#define __TTF_SPI_SCREEN_H__

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

void dgx_spi_sync_send_cmd(dgx_screen_t *scr, const uint8_t cmd);
void dgx_spi_sync_send_commans(dgx_screen_t *scr, const uint8_t * cmds, int len);
void dgx_spi_sync_send_data(dgx_screen_t *scr, const uint8_t *data, int len);
esp_err_t dgx_spi_init(dgx_screen_t *scr, spi_host_device_t host_id, gpio_num_t mosi, gpio_num_t sclk, gpio_num_t cs, gpio_num_t dc, int clock_speed_hz);
void dgx_spi_wait_pending(dgx_screen_t *scr);
void dgx_spi_draw_pixel(dgx_screen_t *scr, int16_t x, int16_t y, uint32_t color);
uint32_t dgx_spi_read_pixel(dgx_screen_t *scr, int16_t x, int16_t y);
void dgx_spi_fill_hline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, uint32_t color, uint32_t bg, uint32_t mask, uint8_t mask_bits);
void dgx_spi_fill_vline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t h, uint32_t color, uint32_t bg, uint32_t mask, uint8_t mask_bits);
void dgx_spi_fill_rectangle(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color);
void dgx_spi_fast_vline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t h, uint32_t color);
void dgx_spi_fast_hline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, uint32_t color);

void dgx_spi_allocate_queue(dgx_screen_t *scr, uint16_t free);
void dgx_spi_crset(dgx_screen_t *scr, uint16_t caset_lo, uint16_t caset_hi, uint16_t raset_lo, uint16_t raset_hi);
void dgx_spi_write_ram(dgx_screen_t *scr);
void dgx_spi_wait_data(dgx_screen_t *scr);
void dgx_spi_write_data(dgx_screen_t *scr, uint8_t *data, uint32_t lenbits);
void dgx_spi_write_value(dgx_screen_t *scr, uint32_t value);

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on

#endif

#endif /* __TTF_SPI_SCREEN_H__ */
