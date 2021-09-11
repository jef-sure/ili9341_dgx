/*
 * MIT License
 *
 * Copyright (c) 2021 Anton Petrusevich
 *
 */

#ifndef _SSD1366_H_
#define _SSD1366_H_
#include "dgx_screen.h"
#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

#define SSD1306_I2C_ADDRESS   0x3C

void dgx_i2c_ssd1306_contrast(dgx_screen_t *scr, uint8_t contrast);
esp_err_t dgx_i2c_ssd1306_init(dgx_screen_t *scr, uint8_t height, uint8_t is_ext_vcc, uint8_t i2c_address);

#ifdef __cplusplus
// @formatter:off
    }
    // @formatter:on
#endif
#endif /* _SSD1366_H_ */
