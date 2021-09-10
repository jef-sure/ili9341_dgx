#ifndef _dgx_P8_ILI_9341_H_
#define _dgx_P8_ILI_9341_H_
#include "dgx_screen.h"

#include "driver/gpio.h"

#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

esp_err_t dgx_p8_ili9341_init(dgx_screen_t *scr, gpio_num_t rst, uint8_t color_bits, dgx_color_order_t cbo);
void dgx_p8_ili9341_orientation(dgx_screen_t *scr, dgx_orientation_t dir_x, dgx_orientation_t dir_y, bool swap_xy);


#ifdef __cplusplus
// @formatter:off
}
// @formatter:on
#endif
#endif /* _dgx_P8_ILI_9341_H_ */
