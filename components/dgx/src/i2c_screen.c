#include <string.h>

#include "dgx_i2c_screen.h"

esp_err_t dgx_i2c_init(dgx_screen_t *scr, i2c_port_t i2c_num, gpio_num_t sda, gpio_num_t sclk, int clock_speed_hz) {

    esp_err_t rc;
    i2c_config_t i2c_config = { //
            .mode = I2C_MODE_MASTER, //
            .sda_io_num = sda, //
            .scl_io_num = sclk, //
            .sda_pullup_en = GPIO_PULLUP_ENABLE, //
            .scl_pullup_en = GPIO_PULLUP_ENABLE, //
            .master.clk_speed = clock_speed_hz //
            };
    rc = i2c_param_config(i2c_num, &i2c_config);
    if (rc != ESP_OK) return rc;
    scr->i2c_num = i2c_num;
    scr->cg_col_shift = 0;
    scr->cg_row_shift = 0;
    scr->bus_type = BusI2C;
    return i2c_driver_install(i2c_num, I2C_MODE_MASTER, 0, 0, 0);
}
