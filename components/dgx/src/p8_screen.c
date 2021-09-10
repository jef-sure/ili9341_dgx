#include <string.h>

#include "dgx_p8_screen.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"

static uint32_t p8_gpio_mask_set[256];
static uint32_t p8_gpio_mask_clear[256];

#define p8_mask_soc(mask, bit) \
    do {\
        if(i & mask)\
            set |= 1 << scr->lcd_d##bit;\
        else \
            clear |= 1 << scr->lcd_d##bit;\
    } while (0)

static void p8_init_masks(dgx_screen_t *scr) {
    uint16_t i;
    for (i = 0; i < 256; ++i) {
        uint32_t set = 0;
        uint32_t clear = 1 << scr->lcd_wr;
        p8_mask_soc(0x80, 7);
        p8_mask_soc(0x40, 6);
        p8_mask_soc(0x20, 5);
        p8_mask_soc(0x10, 4);
        p8_mask_soc(0x08, 3);
        p8_mask_soc(0x04, 2);
        p8_mask_soc(0x02, 1);
        p8_mask_soc(0x01, 0);
        p8_gpio_mask_set[i] = set;
        p8_gpio_mask_clear[i] = clear;
    }
}

static inline void p8_set_cmd(dgx_screen_t *scr) {
//    gpio_set_level(scr->dc, 0);
    GPIO.out_w1tc = 1 << scr->dc;

}

static inline void p8_set_data(dgx_screen_t *scr) {
//    gpio_set_level(scr->dc, 1);
    GPIO.out_w1ts = 1 << scr->dc;
}

static inline void p8_send_byte(dgx_screen_t *scr, const uint8_t cmd) {
    /*
     gpio_set_level(scr->lcd_wr, 0);
     gpio_set_level(scr->lcd_d7, (cmd & 0x80) >> 7);
     gpio_set_level(scr->lcd_d6, (cmd & 0x40) >> 6);
     gpio_set_level(scr->lcd_d5, (cmd & 0x20) >> 5);
     gpio_set_level(scr->lcd_d4, (cmd & 0x10) >> 4);
     gpio_set_level(scr->lcd_d3, (cmd & 0x08) >> 3);
     gpio_set_level(scr->lcd_d2, (cmd & 0x04) >> 2);
     gpio_set_level(scr->lcd_d1, (cmd & 0x02) >> 1);
     gpio_set_level(scr->lcd_d0, (cmd & 0x01) >> 0);
     gpio_set_level(scr->lcd_wr, 1);
     */
    GPIO.out_w1tc = p8_gpio_mask_clear[cmd];
    uint32_t bm = p8_gpio_mask_set[cmd];
    if (bm) GPIO.out_w1ts = bm;
    GPIO.out_w1ts = 1 << scr->lcd_wr;
}

void dgx_p8_sync_send_cmd(dgx_screen_t *scr, const uint8_t cmd) {
    p8_set_cmd(scr);
    p8_send_byte(scr, cmd);
}

void dgx_p8_sync_send_data(dgx_screen_t *scr, const uint8_t *data, int len) {
    p8_set_data(scr);
    while (len--) {
        p8_send_byte(scr, *data++);
    }
}

void dgx_p8_set_area(dgx_screen_t *scr, uint16_t caset_lo, uint16_t caset_hi, uint16_t raset_lo, uint16_t raset_hi) {
    dgx_p8_crset(scr, caset_lo, caset_hi, raset_lo, raset_hi);
    dgx_p8_write_ram(scr);
}

void dgx_p8_update_screen(dgx_screen_t *scr, int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    return;
}

static void dgx_p8_init_transactions(dgx_screen_t *scr) {
    for (uint16_t i = 0; i < 5; i++) {
        if ((i & 1) == 0) {
            //Even transfers are commands
            scr->trans[i].length = 8;
            scr->trans[i].user = 0;
        } else {
            //Odd transfers are data
            scr->trans[i].length = 8 * 4;
            scr->trans[i].user = 0;
        }
        scr->trans[i].flags = SPI_TRANS_USE_TXDATA;
    }
    scr->trans_data->user = 0;
}

esp_err_t dgx_p8_init(dgx_screen_t *scr, gpio_num_t lcd_d0, gpio_num_t lcd_d1, gpio_num_t lcd_d2, gpio_num_t lcd_d3,
        gpio_num_t lcd_d4, gpio_num_t lcd_d5, gpio_num_t lcd_d6, gpio_num_t lcd_d7, gpio_num_t lcd_wr,
        gpio_num_t lcd_rd, gpio_num_t cs, gpio_num_t dc) {
    scr->bus_type = BusP8;
    scr->cg_col_shift = 0;
    scr->cg_row_shift = 0;
    scr->lcd_d0 = lcd_d0;
    scr->lcd_d1 = lcd_d1;
    scr->lcd_d2 = lcd_d2;
    scr->lcd_d3 = lcd_d3;
    scr->lcd_d4 = lcd_d4;
    scr->lcd_d5 = lcd_d5;
    scr->lcd_d6 = lcd_d6;
    scr->lcd_d7 = lcd_d7;
    scr->lcd_wr = lcd_wr;
    scr->lcd_rd = lcd_rd;
    scr->cs = cs;
    scr->dc = dc;
    p8_init_masks(scr);
    scr->trans = heap_caps_calloc(6, sizeof(spi_transaction_t), MALLOC_CAP_DMA);
    if (!scr->trans) return ESP_FAIL;
    scr->trans_data = scr->trans + 5;
    dgx_p8_init_transactions(scr);
    scr->pending_transactions = 0;
    scr->max_transactions = 20;
    scr->is_data_free = true;
    scr->in_progress = 0;
    scr->set_area = dgx_p8_set_area;
    scr->write_area = dgx_p8_write_data;
    scr->write_value = dgx_p8_write_value;
    scr->wait_screen_buffer = dgx_p8_wait_data;
    scr->draw_pixel = dgx_p8_draw_pixel;
    scr->read_pixel = dgx_p8_read_pixel;
    scr->fill_hline = dgx_p8_fill_hline;
    scr->fill_vline = dgx_p8_fill_vline;
    scr->fast_hline = dgx_p8_fast_hline;
    scr->fast_vline = dgx_p8_fast_vline;
    scr->fill_rectangle = dgx_p8_fill_rectangle;
    scr->update_screen = dgx_p8_update_screen;
    gpio_pad_select_gpio(dc);
    gpio_pad_select_gpio(cs);
    gpio_pad_select_gpio(lcd_d0);
    gpio_pad_select_gpio(lcd_d1);
    gpio_pad_select_gpio(lcd_d2);
    gpio_pad_select_gpio(lcd_d3);
    gpio_pad_select_gpio(lcd_d4);
    gpio_pad_select_gpio(lcd_d5);
    gpio_pad_select_gpio(lcd_d6);
    gpio_pad_select_gpio(lcd_d7);
    gpio_pad_select_gpio(lcd_wr);
    gpio_set_direction(dc, GPIO_MODE_OUTPUT);
    gpio_set_direction(cs, GPIO_MODE_OUTPUT);
    gpio_set_direction(lcd_d0, GPIO_MODE_OUTPUT);
    gpio_set_direction(lcd_d1, GPIO_MODE_OUTPUT);
    gpio_set_direction(lcd_d2, GPIO_MODE_OUTPUT);
    gpio_set_direction(lcd_d3, GPIO_MODE_OUTPUT);
    gpio_set_direction(lcd_d4, GPIO_MODE_OUTPUT);
    gpio_set_direction(lcd_d5, GPIO_MODE_OUTPUT);
    gpio_set_direction(lcd_d6, GPIO_MODE_OUTPUT);
    gpio_set_direction(lcd_d7, GPIO_MODE_OUTPUT);
    gpio_set_direction(lcd_wr, GPIO_MODE_OUTPUT);
    if (lcd_rd >= 0) {
        gpio_pad_select_gpio(lcd_rd);
        gpio_set_direction(lcd_rd, GPIO_MODE_OUTPUT);
        gpio_set_level(lcd_rd, 1);
    }
    gpio_set_level(cs, 0);
    gpio_set_level(lcd_wr, 1);
    return ESP_OK;
}

void dgx_p8_crset(dgx_screen_t *scr, uint16_t caset_lo, uint16_t caset_hi, uint16_t raset_lo, uint16_t raset_hi) {
    if (scr->fb_col_hi != caset_hi || scr->fb_col_lo != caset_lo) {
        dgx_p8_sync_send_cmd(scr, scr->trans[0].tx_data[0]);
        scr->trans[1].tx_data[0] = caset_lo >> 8;
        scr->trans[1].tx_data[1] = caset_lo & 255;
        scr->trans[1].tx_data[2] = caset_hi >> 8;
        scr->trans[1].tx_data[3] = caset_hi & 255;
        scr->fb_col_lo = caset_lo;
        scr->fb_col_hi = caset_hi;
        dgx_p8_sync_send_data(scr, scr->trans[1].tx_data, 4);
    }
    if (scr->fb_row_hi != raset_hi || scr->fb_row_lo != raset_lo) {
        dgx_p8_sync_send_cmd(scr, scr->trans[2].tx_data[0]);
        scr->trans[3].tx_data[0] = raset_lo >> 8;
        scr->trans[3].tx_data[1] = raset_lo & 255;
        scr->trans[3].tx_data[2] = raset_hi >> 8;
        scr->trans[3].tx_data[3] = raset_hi & 255;
        scr->fb_row_lo = raset_lo;
        scr->fb_row_hi = raset_hi;
        dgx_p8_sync_send_data(scr, scr->trans[3].tx_data, 4);
    }
}

void dgx_p8_write_ram(dgx_screen_t *scr) {
    dgx_p8_sync_send_cmd(scr, scr->trans[4].tx_data[0]);
}

void dgx_p8_write_data(dgx_screen_t *scr, uint8_t *data, uint32_t lenbits) {
    dgx_p8_sync_send_data(scr, data, (lenbits + 7) / 8);
}

void dgx_p8_write_value(dgx_screen_t *scr, uint32_t value) {
    uint32_t length = scr->lcd_ram_bits + (7 & (8 - (scr->lcd_ram_bits & 7)));
    dgx_fill_buf_value(scr->trans_data->tx_data, 0, value, scr->lcd_ram_bits);
    dgx_p8_sync_send_data(scr, scr->trans_data->tx_data, (length + 7) / 8);
}

void dgx_p8_wait_data(dgx_screen_t *scr) {
}

void dgx_p8_draw_pixel(dgx_screen_t *scr, int16_t x, int16_t y, uint32_t color) {
    if (x < 0 || y < 0 || x >= scr->width || y >= scr->height) return;
    dgx_p8_crset(scr, x, x, y, y);
    dgx_p8_write_ram(scr);
    dgx_p8_write_value(scr, color);
}

uint32_t dgx_p8_read_pixel(dgx_screen_t *scr, int16_t x, int16_t y) {
    return 0;
}

void dgx_p8_fill_line_buffer(dgx_screen_t *scr, int16_t w, uint32_t color, uint32_t bg, uint32_t mask,
        uint8_t mask_bits) {
    const uint32_t test_bit = 1;
    uint8_t *lp = scr->draw_buffer;
    if (scr->color_bits == 16) {
        color &= 0xffff;
        bg &= 0xffff;
        for (; w-- > 0; mask = ror_bits(mask, mask_bits)) {
            uint16_t c = mask & test_bit ? (uint16_t) color : (uint16_t) bg;
            *lp++ = c >> 8;
            *lp++ = c & 255;
        }
    } else if (scr->color_bits == 18) {
        color &= 0xffffff;
        bg &= 0xffffff;
        for (; w-- > 0; mask = ror_bits(mask, mask_bits)) {
            uint32_t c = mask & test_bit ? color : bg;
            *lp++ = c >> 16;
            *lp++ = c >> 8;
            *lp++ = c & 255;
        }
    } else if (scr->color_bits == 12) {
        color &= 0xfff0u;
        bg &= 0xfff0u;
        for (uint16_t i = 0; w-- > 0; ++i, mask = ror_bits(mask, mask_bits)) {
            uint16_t c = mask & test_bit ? (uint16_t) color : (uint16_t) bg;
            lp = dgx_fill_buf_value_12(lp, i, c);
        }
    }
}

static void dgx_p8_send_line(dgx_screen_t *scr, int16_t w, uint32_t color, uint32_t bg, uint32_t mask,
        uint8_t mask_bits) {
    dgx_p8_fill_line_buffer(scr, w, color, bg, mask, mask_bits);
    dgx_p8_write_ram(scr);
    dgx_p8_write_data(scr, scr->draw_buffer, scr->lcd_ram_bits * w);
}

void dgx_p8_fill_hline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, uint32_t color, uint32_t bg, uint32_t mask,
        uint8_t mask_bits) {
    if (y < 0 || y >= scr->height || x + w <= 0 || x >= scr->width) return;
    if (x < 0) {
        w = x + w;
        x = 0;
    }
    if (x + w > scr->width) {
        w = scr->width - x;
    }
    dgx_p8_crset(scr, x, x + w - 1, y, y);
    dgx_p8_send_line(scr, w, color, bg, mask, mask_bits);
}

void dgx_p8_fill_vline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t h, uint32_t color, uint32_t bg, uint32_t mask,
        uint8_t mask_bits) {
    if (x < 0 || x >= scr->width || y + h <= 0 || y >= scr->height) return;
    if (y < 0) {
        h = y + h;
        h = 0;
    }
    if (y + h > scr->height) {
        h = scr->width - y;
    }
    dgx_p8_crset(scr, x, x, y, y + h - 1);
    dgx_p8_send_line(scr, h, color, bg, mask, mask_bits);
}

void dgx_p8_fill_rectangle(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color) {
    if (y < 0 || w <= 0 || x + w <= 0 || x >= scr->width || h <= 0 || y + h <= 0 || y >= scr->height) return;
    if (x < 0) {
        w = x + w;
        x = 0;
    }
    if (x + w > scr->width) {
        w = scr->width - x;
    }
    if (y < 0) {
        h = y + h;
        y = 0;
    }
    if (y + h > scr->height) {
        h = scr->height - y;
    }
    dgx_p8_crset(scr, x, x + w - 1, y, y + h - 1);
    dgx_p8_write_ram(scr);
    uint8_t blen = (scr->lcd_ram_bits + (7 & (8 - (scr->lcd_ram_bits & 7)))) / 8;
    dgx_fill_buf_value(scr->trans_data->tx_data, 0, color, scr->lcd_ram_bits);
    int32_t sz = (int32_t) w * (int32_t) h;
    p8_set_data(scr);
    if (scr->lcd_ram_bits == 16) {
        if (scr->trans_data->tx_data[0] == scr->trans_data->tx_data[1]) {
            uint8_t scb = scr->trans_data->tx_data[0];
            sz *= 2;
            while (sz--) {
                p8_send_byte(scr, scb);
            }
        } else {
            uint8_t fcb = scr->trans_data->tx_data[0];
            uint8_t scb = scr->trans_data->tx_data[1];
            while (sz--) {
                p8_send_byte(scr, fcb);
                p8_send_byte(scr, scb);
            }

        }
    } else {
        while (sz--) {
            for (uint8_t i = 0; i < blen; ++i) {
                p8_send_byte(scr, scr->trans_data->tx_data[i]);
            }
        }
    }
}

static void dgx_p8_send_fast_line(dgx_screen_t *scr, int16_t w, uint32_t color) {
    uint8_t blen = (scr->lcd_ram_bits + (7 & (8 - (scr->lcd_ram_bits & 7)))) / 8;
    dgx_fill_buf_value(scr->trans_data->tx_data, 0, color, scr->lcd_ram_bits);
    dgx_p8_write_ram(scr);
    p8_set_data(scr);
    if (scr->lcd_ram_bits == 16) {
        if (scr->trans_data->tx_data[0] == scr->trans_data->tx_data[1]) {
            uint8_t scb = scr->trans_data->tx_data[0];
            w *= 2;
            while (w--) {
                p8_send_byte(scr, scb);
            }
        } else {
            uint8_t fcb = scr->trans_data->tx_data[0];
            uint8_t scb = scr->trans_data->tx_data[1];
            while (w--) {
                p8_send_byte(scr, fcb);
                p8_send_byte(scr, scb);
            }

        }
    } else {
        while (w--) {
            for (uint8_t i = 0; i < blen; ++i) {
                p8_send_byte(scr, scr->trans_data->tx_data[i]);
            }
        }
    }
}

void dgx_p8_fast_vline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t h, uint32_t color) {
    if (x < 0 || x >= scr->width || y + h <= 0 || y >= scr->height) return;
    if (y < 0) {
        h = y + h;
        h = 0;
    }
    if (y + h > scr->height) {
        h = scr->width - y;
    }
    dgx_p8_crset(scr, x, x, y, y + h - 1);
    dgx_p8_send_fast_line(scr, h, color);

}

void dgx_p8_fast_hline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, uint32_t color) {
    if (y < 0 || y >= scr->height || x + w <= 0 || x >= scr->width) return;
    if (x < 0) {
        w = x + w;
        x = 0;
    }
    if (x + w > scr->width) {
        w = scr->width - x;
    }
    dgx_p8_crset(scr, x, x + w - 1, y, y);
    dgx_p8_send_fast_line(scr, w, color);
}

