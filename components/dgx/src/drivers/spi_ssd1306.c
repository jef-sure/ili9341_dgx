#include <string.h>

#include "dgx_spi_screen.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "dgx_bw_screen.h"

// Control byte
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

#define SSD1306_CMD_DEACTIVATE_SCROLL      0x2E    //

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

typedef struct {
    uint8_t cmd;
    uint8_t databytes; //No of data in data; 0xFF = end of cmds.
    uint8_t delay;
    uint8_t data[16];
} lcd_init_cmd_t;

DRAM_ATTR static const
lcd_init_cmd_t st_init_cmds[] = {
//
        { SSD1306_CMD_DISPLAY_OFF, 0, 0, { 0 } }, //
        { SSD1306_CMD_SET_MUX_RATIO, 1, 0, { 31 } }, //
        { SSD1306_CMD_SET_DISPLAY_OFFSET, 1, 0, { 0 } }, //
        { SSD1306_CMD_SET_DISPLAY_START_LINE, 0, 0, { 0 } }, //
        { SSD1306_CMD_DEACTIVATE_SCROLL, 0, 0, { 0 } }, //
        { SSD1306_CMD_SET_SEGMENT_REMAP, 0, 0, { 0 } }, //
        { SSD1306_CMD_SET_COM_SCAN_MODE, 0, 0, { 0 } }, //
        { SSD1306_CMD_SET_COM_PIN_MAP, 1, 0, { 0x02 } }, //
        { SSD1306_CMD_SET_CONTRAST, 1, 0, { 0x8F } }, //
        { SSD1306_CMD_DISPLAY_RAM, 0, 0, { 0 } }, //
        { SSD1306_CMD_DISPLAY_NORMAL, 0, 0, { 0 } }, //
        { SSD1306_CMD_SET_DISPLAY_CLK_DIV, 1, 0, { 0x80 } }, //
        { SSD1306_CMD_SET_CHARGE_PUMP, 1, 0, { 0x14 } }, //
        { SSD1306_CMD_SET_MEMORY_ADDR_MODE, 1, 0, { 0 } }, //
        { SSD1306_CMD_SET_PRECHARGE, 1, 0, { 0xF1 } }, //
        { SSD1306_CMD_SET_VCOMH_DESELCT, 1, 0, { 0x40 } }, //
        { SSD1306_CMD_DISPLAY_ON, 0, 0, { 0 } }, //
        { 0, 0xff, 0, { 0 } } // Stop

};

void dgx_spi_ssd1306_update_screen(dgx_screen_t *scr, int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
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
    uint8_t page_s = (uint16_t) y1 >> 3;
    uint8_t page_e = (uint16_t) y2 >> 3;
    uint8_t clo = scr->height == 48 ? x1 + 32 : x1;
    uint8_t chi = scr->height == 48 ? x2 + 32 : x2;
    uint8_t crset[6] = {
    //
            SSD1306_CMD_SET_COLUMN_RANGE,//
            clo, chi, //
            SSD1306_CMD_SET_PAGE_RANGE, //
            page_s, page_e //
            };
    dgx_spi_sync_send_commans(scr, crset, 6);
    dgx_spi_wait_data(scr);
    if (x1 == 0 && x2 == (scr->width - 1) && page_s == 0 && page_e == ((uint16_t) (scr->height - 1) >> 3)) {
        uint8_t *pb = scr->frame_buffer;
        uint32_t bsize = scr->width * (page_e + 1);
        while (bsize != 0) {
            uint32_t byte_len = bsize > scr->max_transfer_sz ? scr->max_transfer_sz : bsize;
            dgx_spi_write_data(scr, pb, byte_len * 8u);
            bsize -= byte_len;
            pb += byte_len;
            dgx_spi_wait_data(scr);
        }
    } else {
        uint32_t xw = x2 - x1 + 1u;
        size_t bidx;
        for (bidx = 0; page_s <= page_e; ++page_s) {
            uint8_t *pb = scr->frame_buffer + page_s * scr->width + x1;
            uint32_t bleft = scr->draw_buffer_len - bidx;
            uint32_t byte_len = bleft > scr->max_transfer_sz ? scr->max_transfer_sz : bleft;
            if (byte_len > xw) byte_len = xw;
            memcpy(scr->draw_buffer + bidx, pb, byte_len);
            bidx += byte_len;
            if (byte_len < xw || bidx == scr->draw_buffer_len) {
                dgx_spi_write_data(scr, scr->draw_buffer, bidx * 8u);
                dgx_spi_wait_data(scr);
                bidx = 0;
                if (byte_len < xw) {
                    memcpy(scr->draw_buffer, pb + byte_len, xw - byte_len);
                    bidx = xw - byte_len;
                }
            }
        }
        if (bidx != 0) {
            dgx_spi_write_data(scr, scr->draw_buffer, bidx * 8u);
            dgx_spi_wait_data(scr);
        }
    }
}

void dgx_spi_ssd1306_contrast(dgx_screen_t *scr, uint8_t contrast) {
    dgx_spi_sync_send_cmd(scr, SSD1306_CMD_SET_CONTRAST);
    dgx_spi_sync_send_cmd(scr, contrast);
}

static void dgx_ssd1306_destroy_screen(dgx_screen_t *scr) {
    dgx_bw_destroy_screen(scr);
    free(scr->trans);
}

esp_err_t dgx_spi_ssd1306_init(dgx_screen_t *scr, gpio_num_t rst, uint8_t height, uint8_t is_ext_vcc) {
    int16_t width;
    esp_err_t ret = spi_bus_add_device(scr->host_id, &scr->devcfg, &scr->spi);
    if (ret != ESP_OK) return ret;
    if (height == 16) {
        width = 96;
    } else if (height == 48) {
        width = 64;
    } else {
        width = 128;
    }
    esp_err_t rc = dgx_bw_init(scr, width, height);
    if (rc != ESP_OK) return rc;
    scr->rgb_order = RGB;
    scr->fb_col_hi = width - 1;
    scr->fb_col_lo = 0;
    scr->fb_row_hi = height - 1;
    scr->fb_row_lo = 0;
    scr->update_screen = dgx_spi_ssd1306_update_screen;
    scr->destroy = dgx_ssd1306_destroy_screen;
    scr->cmd_col_set = scr->trans[0].tx_data[0] = SSD1306_CMD_SET_COLUMN_RANGE;           //Column Address Set
    scr->cmd_row_set = scr->trans[2].tx_data[0] = SSD1306_CMD_SET_PAGE_RANGE;           //Page address set
    scr->cmd_write_data = scr->trans[4].tx_data[0] = 0;           //memory write
    const lcd_init_cmd_t *lcd_init_cmds = st_init_cmds;

//Initialize non-SPI GPIOs
    gpio_set_direction(rst, GPIO_MODE_OUTPUT);

//Reset the display
    gpio_set_level(rst, 1);
    vTaskDelay(10 / portTICK_RATE_MS);
    gpio_set_level(rst, 0);
    vTaskDelay(10 / portTICK_RATE_MS);
    gpio_set_level(rst, 1);

//Send all the commands
    uint8_t adj_data[1];
    for (uint8_t cidx = 0; lcd_init_cmds[cidx].databytes != 0xff; ++cidx) {
        uint8_t cmd = lcd_init_cmds[cidx].cmd;
        dgx_spi_sync_send_cmd(scr, cmd);
        uint8_t const *data = lcd_init_cmds[cidx].data;
        if (cmd == SSD1306_CMD_SET_CHARGE_PUMP) {
            data = adj_data;
            if (is_ext_vcc) {
                *adj_data = 0x10;
            } else {
                *adj_data = 0x14;
            }
        } else if (cmd == SSD1306_CMD_SET_MUX_RATIO) {
            *adj_data = height - 1;
            data = adj_data;
        } else if (cmd == SSD1306_CMD_SET_COM_PIN_MAP) {
            if (height == 64 || height == 48) {
                *adj_data = 0x12;
                data = adj_data;
            }
        } else if (cmd == SSD1306_CMD_SET_CONTRAST) {
            if (height == 64 || height == 48) {
                data = adj_data;
                if (is_ext_vcc) {
                    *adj_data = 0x9f;
                } else {
                    *adj_data = 0xcf;
                }
            } else if (height != 32) {
                data = adj_data;
                if (is_ext_vcc) {
                    *adj_data = 0x10;
                } else {
                    *adj_data = 0xaf;
                }
            }
        } else if (cmd == SSD1306_CMD_SET_PRECHARGE) {
            data = adj_data;
            if (is_ext_vcc) {
                *adj_data = 0x22;
            } else {
                *adj_data = 0xF1;
            }
        }
        if (lcd_init_cmds[cidx].databytes != 0) {
            dgx_spi_sync_send_cmd(scr, *data);
        }
//        dgx_spi_sync_send_data(scr, data, lcd_init_cmds[cidx].databytes);
        if (lcd_init_cmds[cidx].delay) {
            int ms = lcd_init_cmds[cidx].delay == 255 ? 500 : lcd_init_cmds[cidx].delay;
            vTaskDelay(ms / portTICK_PERIOD_MS);
        }
    }
    return ESP_OK;
}

