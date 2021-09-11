/*
 * MIT License
 *
 * Copyright (c) 2021 Anton Petrusevich
 *
 */

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "dgx_spi_screen.h"

// ST7735 specific commands used in init
#define ST7735_NOP          0x00
#define ST7735_SWRESET      0x01
#define ST7735_RDDID        0x04
#define ST7735_RDDST        0x09

#define ST7735_SLPIN        0x10
#define ST7735_SLPOUT       0x11
#define ST7735_PTLON        0x12
#define ST7735_NORON        0x13

#define ST7735_INVOFF       0x20
#define ST7735_INVON        0x21
#define ST7735_DISPOFF      0x28
#define ST7735_DISPON       0x29
#define ST7735_CASET        0x2A
#define ST7735_RASET        0x2B
#define ST7735_RAMWR        0x2C
#define ST7735_RAMRD        0x2E

#define ST7735_PTLAR        0x30
#define ST7735_VSCRDEF      0x33
#define ST7735_COLMOD       0x3A
#define ST7735_MADCTL       0x36
#define ST7735_VSCRSADD     0x37

#define ST7735_FRMCTR1      0xB1
#define ST7735_FRMCTR2      0xB2
#define ST7735_FRMCTR3      0xB3
#define ST7735_INVCTR       0xB4
#define ST7735_DISSET5      0xB6

#define ST7735_PWCTR1       0xC0
#define ST7735_PWCTR2       0xC1
#define ST7735_PWCTR3       0xC2
#define ST7735_PWCTR4       0xC3
#define ST7735_PWCTR5       0xC4
#define ST7735_VMCTR1       0xC5

#define ST7735_RDID1        0xDA
#define ST7735_RDID2        0xDB
#define ST7735_RDID3        0xDC
#define ST7735_RDID4        0xDD

#define ST7735_PWCTR6       0xFC

#define ST7735_GMCTRP1      0xE0
#define ST7735_GMCTRN1      0xE1

#define ST77XX_MADCTL_MY    0x80
#define ST77XX_MADCTL_MX    0x40
#define ST77XX_MADCTL_MV    0x20
#define ST77XX_MADCTL_ML    0x10
#define ST77XX_MADCTL_RGB   0x00
#define ST77XX_MADCTL_BGR   0x08

typedef struct {
    uint8_t cmd;
    uint8_t databytes; //No of data in data; 0xFF = end of cmds.
    uint8_t delay;
    uint8_t data[16];
} lcd_init_cmd_t;

DRAM_ATTR static const
lcd_init_cmd_t st_init_cmds[] = {
//
        { ST7735_SWRESET, 0, 150, { 0 } }, //
        { ST7735_SLPOUT, 0, 255, { 0 } }, //
        { ST7735_FRMCTR1, 3, 0, { 0x01, 0x2C, 0x2D } }, //
        { ST7735_FRMCTR2, 3, 0, { 0x01, 0x2C, 0x2D } }, //
        { ST7735_FRMCTR3, 6, 0, { 0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D } }, //
        { ST7735_INVCTR, 1, 0, { 0x07 } }, //
        { ST7735_PWCTR1, 3, 0, { 0xA2, 0x02, 0x84 } }, //
        { ST7735_PWCTR2, 1, 0, { 0xC5 } }, //
        { ST7735_PWCTR3, 2, 0, { 0x0A, 0x00 } }, //
        { ST7735_PWCTR4, 2, 0, { 0x8A, 0x2A } }, //
        { ST7735_PWCTR5, 2, 0, { 0x8A, 0xEE } }, //
        { ST7735_VMCTR1, 1, 9, { 0x0E } }, //
        { ST7735_MADCTL, 1, 0, { 0x00 | 0x00 } }, // MY=0, MX=0, MV=0, ML=0, MH=0 | 00 - RGB, 08 - BGR
        { ST7735_COLMOD, 1, 0, { 0x06 } }, // Bits per Pixel: 3 - 12, 5 - 16, 6 - 18
        { ST7735_CASET, 4, 0, { 0x00, 0x00, 0x00, 0x7F } }, //
        { ST7735_RASET, 4, 0, { 0x00, 0x00, 0x00, 0x9F } }, //
        { ST7735_GMCTRP1, 16, 0, { 0x02, 0x1c, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2d, 0x29, 0x25, 0x2B, 0x39, 0x00, 0x01,
                                   0x03, 0x10 } }, //
        { ST7735_GMCTRN1, 16, 0, { 0x03, 0x1d, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D, 0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00,
                                   0x02, 0x10 } }, //
        { ST7735_NORON, 0, 10, { 0 } }, //
        { ST7735_DISPON, 0, 100, { 0 } }, //  4: Main screen turn on, no args w/delay
        { 0, 0xff, 0, { 0 } } // Stop
};

void dgx_st7735_destroy_screen(dgx_screen_t *scr) {
    free(scr->draw_buffer);
    free(scr->trans);
}

//Initialize the display
esp_err_t dgx_st7735_init(dgx_screen_t *scr, gpio_num_t rst, uint8_t color_bits, dgx_color_order_t cbo) {
    esp_err_t ret = spi_bus_add_device(scr->host_id, &scr->devcfg, &scr->spi);
    if (ret != ESP_OK) return ret;
    scr->width = 128;
    scr->height = 160;
    scr->color_bits = color_bits;
    scr->lcd_ram_bits = scr->color_bits > 16 ? 24 : (scr->color_bits == 12 ? 12 : 16);
    scr->dir_x = LeftRight;
    scr->dir_y = TopBottom;
    scr->swap_xy = false;
    scr->rgb_order = cbo;
    scr->fb_col_hi = 0x007f;
    scr->fb_col_lo = 0x0000;
    scr->fb_row_hi = 0x009f;
    scr->fb_row_lo = 0x0000;
    scr->draw_buffer_len = 4000;
    scr->draw_buffer = heap_caps_calloc(1, scr->draw_buffer_len, MALLOC_CAP_DMA);
    if (!scr->draw_buffer) return ESP_FAIL;
    scr->destroy = dgx_st7735_destroy_screen;
    scr->frame_buffer = 0;
    scr->cmd_col_set = scr->trans[0].tx_data[0] = ST7735_CASET;           //Column Address Set
    scr->cmd_row_set = scr->trans[2].tx_data[0] = ST7735_RASET;           //Page address set
    scr->cmd_write_data = scr->trans[4].tx_data[0] = ST7735_RAMWR;           //memory write

    const lcd_init_cmd_t *lcd_init_cmds = st_init_cmds;

//Initialize non-SPI GPIOs
    gpio_set_direction(rst, GPIO_MODE_OUTPUT);

//Reset the display
    gpio_set_level(rst, 0);
    vTaskDelay(100 / portTICK_RATE_MS);
    gpio_set_level(rst, 1);
    vTaskDelay(100 / portTICK_RATE_MS);

//Send all the commands
    uint8_t adj_data[1];
    for (uint8_t cidx = 0; lcd_init_cmds[cidx].databytes != 0xff; ++cidx) {
        uint8_t cmd = lcd_init_cmds[cidx].cmd;
        dgx_spi_sync_send_cmd(scr, cmd);
        uint8_t const *data = lcd_init_cmds[cidx].data;
        if (cmd == ST7735_MADCTL) {
            if (cbo == RGB)
                *adj_data = *data & ~0x8;
            else
                *adj_data = *data | 0x08;
            data = adj_data;
        } else if (cmd == ST7735_COLMOD) {
            data = adj_data;
            if (color_bits == 12)
                *adj_data = 0x03;
            else if (color_bits == 18)
                *adj_data = 0x06;
            else
                *adj_data = 0x05;
        }
        dgx_spi_sync_send_data(scr, data, lcd_init_cmds[cidx].databytes);
        if (lcd_init_cmds[cidx].delay) {
            int ms = lcd_init_cmds[cidx].delay == 255 ? 500 : lcd_init_cmds[cidx].delay;
            vTaskDelay(ms / portTICK_PERIOD_MS);
        }
    }
    return ESP_OK;
}

void dgx_st7735_orientation(dgx_screen_t *scr, dgx_orientation_t dir_x, dgx_orientation_t dir_y, bool swap_xy) {
    uint8_t cmd = ST7735_MADCTL;
    uint8_t data[1] = { 0 };
    if (dir_x == RightLeft) data[0] |= ST77XX_MADCTL_MX;
    if (dir_y == BottomTop) data[0] |= ST77XX_MADCTL_MY;
    if (swap_xy) data[0] |= ST77XX_MADCTL_MV;
    if (scr->rgb_order == BGR) data[0] |= ST77XX_MADCTL_BGR;
    scr->dir_x = dir_x;
    scr->dir_y = dir_y;
    scr->swap_xy = swap_xy;
    dgx_spi_sync_send_cmd(scr, cmd);
    dgx_spi_sync_send_data(scr, data, 1);
}
