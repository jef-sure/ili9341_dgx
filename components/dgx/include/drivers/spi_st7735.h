#ifndef _dgx_SPI_ST7735_H_
#define _dgx_SPI_ST7735_H_
#include "dgx_screen.h"
#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

esp_err_t dgx_st7735_init(dgx_screen_t *scr, gpio_num_t rst, uint8_t color_bits, dgx_color_order_t cbo);
void dgx_st7735_orientation(dgx_screen_t *scr, dgx_orientation_t dir_x, dgx_orientation_t dir_y, bool swap_xy);

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on
#endif
#endif /* _dgx_SPI_ST7735_H_ */
