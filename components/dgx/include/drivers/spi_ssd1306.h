#ifndef _SPI_SSD1366_H_
#define _SPI_SSD1366_H_
#include "dgx_screen.h"
#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

void dgx_spi_ssd1306_contrast(dgx_screen_t *scr, uint8_t contrast);
esp_err_t dgx_spi_ssd1306_init(dgx_screen_t *scr, uint8_t height, uint8_t is_ext_vcc, gpio_num_t rst);

#ifdef __cplusplus
// @formatter:off
    }
    // @formatter:on
#endif
#endif /* _SPI_SSD1366_H_ */
