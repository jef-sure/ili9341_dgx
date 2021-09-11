/*
 * MIT License
 *
 * Copyright (c) 2021 Anton Petrusevich
 *
 */

#include <string.h>

#include "esp_heap_caps.h"
#include "dgx_screen.h"
#include "drivers/i2c_ssd1306.h"
#include "dgx_bw_screen.h"

/// Control byte
#define SSD1306_CONTROL_BYTE_CMD_SINGLE    0x80
#define SSD1306_CONTROL_BYTE_CMD_STREAM    0x00
#define SSD1306_CONTROL_BYTE_DATA_STREAM   0x40

// Fundamental commands (pg.28)
#define SSD1306_CMD_SET_CONTRAST           0x81    // follow with 0x7F
#define SSD1306_CMD_DISPLAY_RAM            0xA4
#define SSD1306_CMD_DISPLAY_ALLON          0xA5
#define SSD1306_CMD_DISPLAY_NORMAL         0xA6
#define SSD1306_CMD_DISPLAY_INVERTED       0xA7
#define SSD1306_CMD_DISPLAY_OFF            0xAE
#define SSD1306_CMD_DISPLAY_ON             0xAF

// Addressing Command Table (pg.30)
#define SSD1306_CMD_SET_MEMORY_ADDR_MODE   0x20    // follow with 0x00 = HORZ mode = Behave like a KS108 graphic LCD
#define SSD1306_CMD_SET_COLUMN_RANGE       0x21    // can be used only in HORZ/VERT mode - follow with 0x00 and 0x7F = COL127
#define SSD1306_CMD_SET_PAGE_RANGE         0x22    // can be used only in HORZ/VERT mode - follow with 0x00 and 0x07 = PAGE7

// Hardware Config (pg.31)
#define SSD1306_CMD_SET_DISPLAY_START_LINE 0x40
#define SSD1306_CMD_SET_SEGMENT_REMAP      0xA1
#define SSD1306_CMD_SET_MUX_RATIO          0xA8    // follow with 0x3F = 64 MUX
#define SSD1306_CMD_SET_COM_SCAN_MODE      0xC8
#define SSD1306_CMD_SET_DISPLAY_OFFSET     0xD3    // follow with 0x00
#define SSD1306_CMD_SET_COM_PIN_MAP        0xDA    // follow with 0x12
#define SSD1306_CMD_NOP                    0xE3    // NOP

// Timing and Driving Scheme (pg.32)
#define SSD1306_CMD_SET_DISPLAY_CLK_DIV    0xD5    // follow with 0x80
#define SSD1306_CMD_SET_PRECHARGE          0xD9    // follow with 0xF1
#define SSD1306_CMD_SET_VCOMH_DESELCT      0xDB    // follow with 0x30

// Charge Pump (pg.62)
#define SSD1306_CMD_SET_CHARGE_PUMP        0x8D    // follow with 0x14
static esp_err_t dgx_i2c_ssd1306_driver_init(dgx_screen_t *scr, uint8_t is_ext_vcc) {
    esp_err_t espRc;
    uint8_t height = scr->height;
    if (height != 64 && height != 32 && height != 16) return ESP_ERR_INVALID_ARG;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    // Init sequence
    i2c_master_write_byte(cmd, (scr->i2c_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, SSD1306_CONTROL_BYTE_CMD_STREAM, true);
    i2c_master_write_byte(cmd, SSD1306_CMD_DISPLAY_OFF, true);
    i2c_master_write_byte(cmd, SSD1306_CMD_SET_DISPLAY_CLK_DIV, true);
    i2c_master_write_byte(cmd, 0x80, true);
    i2c_master_write_byte(cmd, SSD1306_CMD_SET_MUX_RATIO, true);
    i2c_master_write_byte(cmd, height - 1, true); //
    i2c_master_write_byte(cmd, SSD1306_CMD_SET_DISPLAY_OFFSET, true);
    i2c_master_write_byte(cmd, 0x0, true);
    i2c_master_write_byte(cmd, SSD1306_CMD_SET_DISPLAY_START_LINE, true);
    i2c_master_write_byte(cmd, SSD1306_CMD_SET_CHARGE_PUMP, true);
    if (is_ext_vcc) {
        i2c_master_write_byte(cmd, 0x10, true);
    } else {
        i2c_master_write_byte(cmd, 0x14, true);
    }
    i2c_master_write_byte(cmd, SSD1306_CMD_SET_SEGMENT_REMAP, true); // reverse left-right mapping | 0xA0
    i2c_master_write_byte(cmd, SSD1306_CMD_SET_COM_SCAN_MODE, true); // reverse up-bottom mapping | 0xC0
    i2c_master_write_byte(cmd, SSD1306_CMD_SET_MEMORY_ADDR_MODE, true);
    i2c_master_write_byte(cmd, 0x0, true); // horizontal
    if (height == 32) {
        i2c_master_write_byte(cmd, SSD1306_CMD_SET_COM_PIN_MAP, true);
        i2c_master_write_byte(cmd, 0x02, true);
        i2c_master_write_byte(cmd, SSD1306_CMD_SET_CONTRAST, true);
        i2c_master_write_byte(cmd, 0x8F, true);
    } else if (height == 64 || height == 48) {
        i2c_master_write_byte(cmd, SSD1306_CMD_SET_COM_PIN_MAP, true);
        i2c_master_write_byte(cmd, 0x12, true);
        i2c_master_write_byte(cmd, SSD1306_CMD_SET_CONTRAST, true);
        if (is_ext_vcc) {
            i2c_master_write_byte(cmd, 0x9f, true);
        } else {
            i2c_master_write_byte(cmd, 0xcf, true);
        }
    } else {
        i2c_master_write_byte(cmd, SSD1306_CMD_SET_COM_PIN_MAP, true);
        i2c_master_write_byte(cmd, 0x02, true);
        i2c_master_write_byte(cmd, SSD1306_CMD_SET_CONTRAST, true);
        if (is_ext_vcc) {
            i2c_master_write_byte(cmd, 0x10, true);
        } else {
            i2c_master_write_byte(cmd, 0xaf, true);
        }
    }
    i2c_master_write_byte(cmd, SSD1306_CMD_SET_PRECHARGE, true);
    if (is_ext_vcc) {
        i2c_master_write_byte(cmd, 0x22, true);
    } else {
        i2c_master_write_byte(cmd, 0xF1, true);
    }
    i2c_master_write_byte(cmd, SSD1306_CMD_SET_VCOMH_DESELCT, true);
    i2c_master_write_byte(cmd, 0x40, true); // no idea
    i2c_master_write_byte(cmd, SSD1306_CMD_DISPLAY_RAM, true);
    i2c_master_write_byte(cmd, SSD1306_CMD_DISPLAY_NORMAL, true);
    i2c_master_write_byte(cmd, SSD1306_CMD_DISPLAY_ON, true);
    i2c_master_stop(cmd);
    espRc = i2c_master_cmd_begin(scr->i2c_num, cmd, 10 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return espRc;
}

static void dgx_i2c_ssd1306_update_screen(dgx_screen_t *scr, int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    if (x1 > x2) {
        int16_t t = x1;
        x1 = x2;
        x2 = t;
    }
    if (y1 > y2) {
        int16_t t = y1;
        y1 = y2;
        y2 = t;
    }
    if (x2 < 0 || y2 < 0 || x1 >= scr->width || y1 >= scr->height) return;
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= scr->width) x2 = scr->width - 1;
    if (y2 >= scr->height) y2 = scr->height - 1;
    for (int16_t y = (y1 & ~7); y <= y2; y += 8) {
        uint8_t page = y / 8;
        uint8_t *pb = scr->frame_buffer + page * scr->width + x1;
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (scr->i2c_address << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, SSD1306_CONTROL_BYTE_CMD_SINGLE, true);
        i2c_master_write_byte(cmd, 0x00 | (x1 & 0xf), true); // reset column
        i2c_master_write_byte(cmd, SSD1306_CONTROL_BYTE_CMD_SINGLE, true);
        i2c_master_write_byte(cmd, 0x10 | ((x1 & 0xf0) >> 4), true);
        i2c_master_write_byte(cmd, SSD1306_CONTROL_BYTE_CMD_SINGLE, true);
        i2c_master_write_byte(cmd, 0xB0 | page, true); // increment page
        i2c_master_write_byte(cmd, SSD1306_CONTROL_BYTE_DATA_STREAM, true);
        i2c_master_write(cmd, pb, x2 - x1 + 1, true);
        i2c_master_stop(cmd);
        i2c_master_cmd_begin(scr->i2c_num, cmd, 10 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
    }
}

void dgx_i2c_ssd1306_contrast(dgx_screen_t *scr, uint8_t contrast) {
    i2c_cmd_handle_t cmd;
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (scr->i2c_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, SSD1306_CONTROL_BYTE_CMD_STREAM, true);
    i2c_master_write_byte(cmd, SSD1306_CMD_SET_CONTRAST, true);
    i2c_master_write_byte(cmd, contrast, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(scr->i2c_num, cmd, 10 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
}

esp_err_t dgx_i2c_ssd1306_init(dgx_screen_t *scr, uint8_t height, uint8_t is_ext_vcc, uint8_t i2c_address) {
    int16_t width;
    if (height == 16) {
        width = 96;
    } else if (height == 48) {
        width = 64;
    } else {
        width = 128;
    }
    esp_err_t rc = dgx_bw_init(scr, width, height);
    if (rc != ESP_OK) return rc;
    scr->update_screen = dgx_i2c_ssd1306_update_screen;
    if (i2c_address == 0) i2c_address = SSD1306_I2C_ADDRESS;
    scr->i2c_address = i2c_address;
    rc = dgx_i2c_ssd1306_driver_init(scr, is_ext_vcc);
    if (rc == ESP_OK) dgx_i2c_update_screen(scr, 0, 0, scr->width - 1, scr->height - 1);
    return rc;
}

