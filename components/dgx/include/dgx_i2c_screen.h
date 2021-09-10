#ifndef __TTF_I2C_SCREEN_H__
#define __TTF_I2C_SCREEN_H__

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

esp_err_t dgx_i2c_init(dgx_screen_t *scr, i2c_port_t i2c_num, gpio_num_t sda, gpio_num_t sclk, int clock_speed_hz);

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on

#endif

#endif /* __TTF_I2C_SCREEN_H__ */
