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

// ST7789 specific commands used in init
#define ST7789_NOP          0x00
#define ST7789_SWRESET      0x01
#define ST7789_RDDID        0x04
#define ST7789_RDDST        0x09

#define ST7789_RDDPM        0x0A      // Read display power mode
#define ST7789_RDD_MADCTL   0x0B      // Read display MADCTL
#define ST7789_RDD_COLMOD   0x0C      // Read display pixel format
#define ST7789_RDDIM        0x0D      // Read display image mode
#define ST7789_RDDSM        0x0E      // Read display signal mode
#define ST7789_RDDSR        0x0F      // Read display self-diagnostic result (ST7789V)

#define ST7789_SLPIN        0x10
#define ST7789_SLPOUT       0x11
#define ST7789_PTLON        0x12
#define ST7789_NORON        0x13

#define ST7789_INVOFF       0x20
#define ST7789_INVON        0x21
#define ST7789_GAMSET       0x26      // Gamma set
#define ST7789_DISPOFF      0x28
#define ST7789_DISPON       0x29
#define ST7789_CASET        0x2A
#define ST7789_RASET        0x2B
#define ST7789_RAMWR        0x2C
#define ST7789_RGBSET       0x2D      // Color setting for 4096, 64K and 262K colors
#define ST7789_RAMRD        0x2E

#define ST7789_PTLAR        0x30
#define ST7789_VSCRDEF      0x33      // Vertical scrolling definition (ST7789V)
#define ST7789_TEOFF        0x34      // Tearing effect line off
#define ST7789_TEON         0x35      // Tearing effect line on
#define ST7789_MADCTL       0x36      // Memory data access control
#define ST7789_IDMOFF       0x38      // Idle mode off
#define ST7789_IDMON        0x39      // Idle mode on
#define ST7789_RAMWRC       0x3C      // Memory write continue (ST7789V)
#define ST7789_RAMRDC       0x3E      // Memory read continue (ST7789V)
#define ST7789_COLMOD       0x3A

#define ST7789_RAMCTRL      0xB0      // RAM control
#define ST7789_RGBCTRL      0xB1      // RGB control
#define ST7789_PORCTRL      0xB2      // Porch control
#define ST7789_FRCTRL1      0xB3      // Frame rate control
#define ST7789_PARCTRL      0xB5      // Partial mode control
#define ST7789_GCTRL        0xB7      // Gate control
#define ST7789_GTADJ        0xB8      // Gate on timing adjustment
#define ST7789_DGMEN        0xBA      // Digital gamma enable
#define ST7789_VCOMS        0xBB      // VCOMS setting
#define ST7789_LCMCTRL      0xC0      // LCM control
#define ST7789_IDSET        0xC1      // ID setting
#define ST7789_VDVVRHEN     0xC2      // VDV and VRH command enable
#define ST7789_VRHS         0xC3      // VRH set
#define ST7789_VDVSET       0xC4      // VDV setting
#define ST7789_VCMOFSET     0xC5      // VCOMS offset set
#define ST7789_FRCTR2       0xC6      // FR Control 2
#define ST7789_CABCCTRL     0xC7      // CABC control
#define ST7789_REGSEL1      0xC8      // Register value section 1
#define ST7789_REGSEL2      0xCA      // Register value section 2
#define ST7789_PWMFRSEL     0xCC      // PWM frequency selection
#define ST7789_PWCTRL1      0xD0      // Power control 1
#define ST7789_VAPVANEN     0xD2      // Enable VAP/VAN signal output
#define ST7789_CMD2EN       0xDF      // Command 2 enable
#define ST7789_PVGAMCTRL    0xE0      // Positive voltage gamma control
#define ST7789_NVGAMCTRL    0xE1      // Negative voltage gamma control
#define ST7789_DGMLUTR      0xE2      // Digital gamma look-up table for red
#define ST7789_DGMLUTB      0xE3      // Digital gamma look-up table for blue
#define ST7789_GATECTRL     0xE4      // Gate control
#define ST7789_SPI2EN       0xE7      // SPI2 enable
#define ST7789_PWCTRL2      0xE8      // Power control 2
#define ST7789_EQCTRL       0xE9      // Equalize time control
#define ST7789_PROMCTRL     0xEC      // Program control
#define ST7789_PROMEN       0xFA      // Program mode enable
#define ST7789_NVMSET       0xFC      // NVM setting
#define ST7789_PROMACT      0xFE      // Program action

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

        /*
         *
         spi_master_write_command(dev, 0x01);    //Software Reset
         delayMS(150);

         spi_master_write_command(dev, 0x11);    //Sleep Out
         delayMS(255);

         spi_master_write_command(dev, 0x3A);    //Interface Pixel Format
         spi_master_write_data_byte(dev, 0x55);
         delayMS(10);

         spi_master_write_command(dev, 0x36);    //Memory Data Access Control
         spi_master_write_data_byte(dev, 0x00);

         spi_master_write_command(dev, 0x2A);    //Column Address Set
         spi_master_write_data_byte(dev, 0x00);
         spi_master_write_data_byte(dev, 0x00);
         spi_master_write_data_byte(dev, 0x00);
         spi_master_write_data_byte(dev, 0xF0);

         spi_master_write_command(dev, 0x2B);    //Row Address Set
         spi_master_write_data_byte(dev, 0x00);
         spi_master_write_data_byte(dev, 0x00);
         spi_master_write_data_byte(dev, 0x00);
         spi_master_write_data_byte(dev, 0xF0);

         spi_master_write_command(dev, 0x21);    //Display Inversion On
         delayMS(10);

         spi_master_write_command(dev, 0x13);    //Normal Display Mode On
         delayMS(10);

         spi_master_write_command(dev, 0x29);    //Display ON
         delayMS(255);
         *
         */

        { ST7789_SWRESET, 0, 150, { 0 } }, //
        { ST7789_SLPOUT, 0, 255, { 0 } }, //
        { ST7789_COLMOD, 1, 10, { 0x55 } }, // Bits per Pixel: 3 - 12, 5 - 16, 6 - 18
        { ST7789_MADCTL, 1, 0, { 0x00 | 0x00 } }, // MY=0, MX=0, MV=0, ML=0, MH=0 | 00 - RGB, 08 - BGR
        { ST7789_CASET, 4, 0, { 0x00, 0x00, 0x00, 0xF0 } }, //
        { ST7789_RASET, 4, 0, { 0x00, 0x00, 0x00, 0xF0 } }, //
        { ST7789_INVON, 0, 10, { 0 } }, //
        { ST7789_NORON, 0, 10, { 0 } }, //
        { ST7789_DISPON, 0, 255, { 0 } }, //  4: Main screen turn on, no args w/delay
        { 0, 0xff, 0, { 0 } } // Stop
};

void dgx_st7789_destroy_screen(dgx_screen_t *scr) {
    free(scr->draw_buffer);
    free(scr->trans);
}

//Initialize the display
esp_err_t dgx_st7789_init(dgx_screen_t *scr, gpio_num_t rst, uint8_t color_bits, dgx_color_order_t cbo) {
    //Attach the LCD to the SPI bus
    scr->devcfg.mode = 2;
    esp_err_t ret = spi_bus_add_device(scr->host_id, &scr->devcfg, &scr->spi);
    if (ret != ESP_OK) return ret;
    scr->width = 240;
    scr->height = 240;
    scr->color_bits = color_bits;
    scr->lcd_ram_bits = scr->color_bits > 16 ? 24 : (scr->color_bits == 12 ? 12 : 16);
    scr->dir_x = LeftRight;
    scr->dir_y = TopBottom;
    scr->swap_xy = false;
    scr->rgb_order = cbo;
    scr->fb_col_hi = 0x00f0;
    scr->fb_col_lo = 0x0000;
    scr->fb_row_hi = 0x00f0;
    scr->fb_row_lo = 0x0000;
    scr->draw_buffer_len = 4000;
    scr->draw_buffer = heap_caps_calloc(1, scr->draw_buffer_len, MALLOC_CAP_DMA);
    if (!scr->draw_buffer) return ESP_FAIL;
    scr->destroy = dgx_st7789_destroy_screen;
    scr->frame_buffer = 0;
    scr->cmd_col_set = scr->trans[0].tx_data[0] = ST7789_CASET;           //Column Address Set
    scr->cmd_row_set = scr->trans[2].tx_data[0] = ST7789_RASET;           //Page address set
    scr->cmd_write_data = scr->trans[4].tx_data[0] = ST7789_RAMWR;           //memory write

    const lcd_init_cmd_t *lcd_init_cmds = st_init_cmds;

    if (rst >= 0) {
        gpio_pad_select_gpio(rst);
        gpio_set_direction(rst, GPIO_MODE_OUTPUT);
        gpio_set_level(rst, 1);
        vTaskDelay(100 / portTICK_RATE_MS);
        gpio_set_level(rst, 0);
        vTaskDelay(100 / portTICK_RATE_MS);
        gpio_set_level(rst, 1);
        vTaskDelay(100 / portTICK_RATE_MS);
    }
//Send all the commands
    uint8_t adj_data[1];
    for (uint8_t cidx = 0; lcd_init_cmds[cidx].databytes != 0xff; ++cidx) {
        uint8_t cmd = lcd_init_cmds[cidx].cmd;
        dgx_spi_sync_send_cmd(scr, cmd);
        uint8_t const *data = lcd_init_cmds[cidx].data;
        if (cmd == ST7789_MADCTL) {
            if (cbo == RGB)
                *adj_data = *data & ~0x8;
            else
                *adj_data = *data | 0x08;
            data = adj_data;
        } else if (cmd == ST7789_COLMOD) {
            data = adj_data;
            if (color_bits == 12)
                *adj_data = 0x33;
            else if (color_bits == 18)
                *adj_data = 0x66;
            else
                *adj_data = 0x55;
        }
        dgx_spi_sync_send_data(scr, data, lcd_init_cmds[cidx].databytes);
        if (lcd_init_cmds[cidx].delay) {
            int ms = lcd_init_cmds[cidx].delay == 255 ? 500 : lcd_init_cmds[cidx].delay;
            vTaskDelay(ms / portTICK_PERIOD_MS);
        }
    }
    return ESP_OK;
}

void dgx_st7789_orientation(dgx_screen_t *scr, dgx_orientation_t dir_x, dgx_orientation_t dir_y, bool swap_xy) {
    uint8_t cmd = ST7789_MADCTL;
    uint8_t data[1] = { 0 };
    if (dir_x == RightLeft) data[0] |= ST77XX_MADCTL_MX;
    if (dir_y == BottomTop) {
        data[0] |= ST77XX_MADCTL_MY;
        scr->cg_row_shift = 80;
    }
    if (swap_xy) data[0] |= ST77XX_MADCTL_MV;
    if (scr->rgb_order == BGR) data[0] |= ST77XX_MADCTL_BGR;
//    scr->dir_x = dir_x;
//    scr->dir_y = dir_y;
//    scr->swap_xy = swap_xy;
    dgx_spi_sync_send_cmd(scr, cmd);
    dgx_spi_sync_send_data(scr, data, 1);
}
