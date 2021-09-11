/*
 * MIT License
 *
 * Copyright (c) 2021 Anton Petrusevich
 *
 */

#include <string.h>
#include <stdlib.h>
#include "esp_heap_caps.h"
#include "dgx_bitmap.h"
#include "dgx_vbit_screen.h"

void dgx_vbit_set_area(struct dgx_screen_ *scr, uint16_t caset_lo, uint16_t caset_hi, uint16_t raset_lo,
        uint16_t raset_hi) {
    scr->fb_col_lo = caset_lo;
    scr->fb_col_hi = caset_hi;
    scr->fb_row_lo = raset_lo;
    scr->fb_row_hi = raset_hi;
    scr->fb_pointer_x = caset_lo;
    scr->fb_pointer_y = raset_lo;
}

static uint8_t dgx_get_bits8(uint8_t *array, size_t idx) {
    size_t bi = idx & 7;
    if (bi == 0) return array[0];
    uint8_t lr = array[0] << bi;
    lr |= array[1] >> (8 - bi);
    return lr;
}

void dgx_vbit_write_data(dgx_screen_t *scr, uint8_t *data, uint32_t lenbits) {
    dgx_bw_bitmap_t *bmap = (dgx_bw_bitmap_t*) scr->frame_buffer;
    uint32_t didx = 0;
    while (lenbits >= 1) {
        if (scr->fb_pointer_x > scr->fb_col_hi) {
            scr->fb_pointer_x = scr->fb_col_lo;
            scr->fb_pointer_y++;
        }
        if (scr->fb_pointer_y > scr->fb_row_hi) {
            scr->fb_pointer_y = scr->fb_row_lo;
        }
        if (lenbits >= 8 && (scr->fb_pointer_x & 7) == 0 && (scr->fb_col_hi - scr->fb_pointer_x >= 7)) {
            dgx_bw_bitmap_set8_bits_(bmap, scr->fb_pointer_x, scr->fb_pointer_y, dgx_get_bits8(data, didx));
            scr->fb_pointer_x += 8;
            lenbits -= 8;
            didx += 8;
            ++data;
        } else {
            uint8_t px = dgx_read_buf_value_1(&data, didx);
            dgx_bw_bitmap_set_pixel(bmap, scr->fb_pointer_x, scr->fb_pointer_y, px);
            didx++;
            scr->fb_pointer_x++;
            lenbits--;
        }
    }
}

void dgx_vbit_write_value(dgx_screen_t *scr, uint32_t value) {
    if (scr->fb_pointer_x >= scr->fb_col_hi) {
        scr->fb_pointer_x = scr->fb_col_lo;
        scr->fb_pointer_y++;
    }
    if (scr->fb_pointer_y >= scr->fb_row_hi) {
        scr->fb_pointer_y = scr->fb_row_lo;
    }
    scr->draw_pixel(scr, scr->fb_pointer_x++, scr->fb_pointer_y, value);
}

void dgx_vbit_draw_pixel(dgx_screen_t *scr, int16_t x, int16_t y, uint32_t color) {
    if (x < 0 || y < 0 || x >= scr->width || y >= scr->height) return;
    dgx_bw_bitmap_t *bmap = (dgx_bw_bitmap_t*) scr->frame_buffer;
    dgx_bw_bitmap_set_pixel(bmap, x, y, color);
}

uint32_t dgx_vbit_read_pixel(dgx_screen_t *scr, int16_t x, int16_t y) {
    if (x < 0 || y < 0 || x >= scr->width || y >= scr->height) return 0;
    dgx_bw_bitmap_t *bmap = (dgx_bw_bitmap_t*) scr->frame_buffer;
    return dgx_bw_bitmap_get_pixel(bmap, x, y);
}

void dgx_vbit_wait_data(dgx_screen_t *scr) {
    return;
}

void dgx_vbit_fill_hline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, uint32_t color, uint32_t bg, uint32_t mask,
        uint8_t mask_bits) {
    const uint32_t test_bit = 1;
    scr->in_progress++;
    for (uint16_t i = 0; i < w; ++i, mask = ror_bits(mask, mask_bits)) {
        uint32_t c = mask & test_bit ? color : bg;
        dgx_vbit_draw_pixel(scr, x + i, y, c);
    }
    if (!--scr->in_progress) scr->update_screen(scr, x, y, x + w - 1, y);
}

void dgx_vbit_fill_vline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t h, uint32_t color, uint32_t bg, uint32_t mask,
        uint8_t mask_bits) {
    const uint32_t test_bit = 1;
    scr->in_progress++;
    for (uint16_t i = 0; i < h; ++i, mask = ror_bits(mask, mask_bits)) {
        uint32_t c = mask & test_bit ? color : bg;
        dgx_vbit_draw_pixel(scr, x, y + i, c);
    }
    if (!--scr->in_progress) scr->update_screen(scr, x, y, x, y + h - 1);
}

void dgx_vbit_fast_vline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t h, uint32_t color) {
    scr->in_progress++;
    for (uint16_t i = 0; i < h; ++i) {
        dgx_vbit_draw_pixel(scr, x, y + i, color);
    }
    if (!scr->in_progress) scr->update_screen(scr, x, y, x, y + h - 1);
}

void dgx_vbit_fast_hline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, uint32_t color) {
    scr->in_progress++;
    for (uint16_t i = 0; i < w; ++i) {
        dgx_vbit_draw_pixel(scr, x + i, y, color);
    }
    if (!scr->in_progress) scr->update_screen(scr, x, y, x + w - 1, y);
}

void dgx_vbit_fill_rectangle(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color) {
    if (y < 0 || w < 0 || x + w < 0 || x >= scr->width || h < 0 || y + h < 0 || y > scr->height) return;
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
    scr->in_progress++;
    if (x == 0 && w == scr->width && y == 0 && h == scr->height) {
        dgx_bw_bitmap_t *bmap = (dgx_bw_bitmap_t*) scr->frame_buffer;
        dgx_bw_bitmap_clear(bmap, !!color);
    } else {
        for (uint16_t i = 0; i < h; ++i) {
            dgx_vbit_fast_hline(scr, x, y + i, w, color);
        }
    }
    if (!--scr->in_progress) scr->update_screen(scr, x, y, x + w - 1, y + h - 1);
}

void dgx_vbit_update_screen(dgx_screen_t *scr, int16_t x1, int16_t y1, int16_t x2, int16_t y2) {

}

void dgx_vbit_destroy_screen(dgx_screen_t *scr) {
    free(scr->draw_buffer);
    dgx_bw_bitmap_t *bmap = (dgx_bw_bitmap_t*) scr->frame_buffer;
    dgx_bw_bitmap_free(bmap);
    free(scr->frame_buffer);
}

void dgx_vbit_to_screen(dgx_screen_t *scr, dgx_screen_t *vbscr, int16_t x, int16_t y, uint32_t color, uint32_t bg_color,
        dgx_orientation_t xdir, dgx_orientation_t ydir, bool swap_xy) {
    dgx_bw_bitmap_t *bmap = (dgx_bw_bitmap_t*) vbscr->frame_buffer;
    dgx_bw_bitmap_to_screen(scr, bmap, x, y, color, bg_color, xdir, ydir, swap_xy);
}

esp_err_t dgx_vbit_init(dgx_screen_t *scr, int16_t width, int16_t height) {
    scr->dc = 0;
    scr->pending_transactions = 0;
    scr->trans = 0;
    scr->trans_data = 0;
    scr->is_data_free = true;
    scr->in_progress = 0;
    scr->width = width;
    scr->height = height;
    scr->i2c_num = 0;
    scr->i2c_address = 0;
    scr->color_bits = 1;
    scr->lcd_ram_bits = 1;
    scr->dir_x = LeftRight;
    scr->dir_y = TopBottom;
    scr->swap_xy = false;
    scr->draw_buffer_len = width + 32;
    scr->draw_buffer = calloc(1, scr->draw_buffer_len);
    if (!scr->draw_buffer) return ESP_FAIL;
    dgx_bw_bitmap_t *bmap = calloc(1, sizeof(dgx_bw_bitmap_t));
    if (!bmap) {
        free(scr->draw_buffer);
        scr->draw_buffer = 0;
        return ESP_FAIL;
    }
    *bmap = dgx_bw_bitmap_allocate(width, height);
    if (!bmap->bitmap) {
        free(scr->draw_buffer);
        free(bmap);
        scr->draw_buffer = 0;
        return ESP_FAIL;
    }
    scr->frame_buffer = (void*) bmap;
    dgx_vbit_set_area(scr, 0, width - 1, 0, height - 1);
    scr->set_area = dgx_vbit_set_area;
    scr->draw_pixel = dgx_vbit_draw_pixel;
    scr->read_pixel = dgx_vbit_read_pixel;
    scr->write_area = dgx_vbit_write_data;
    scr->write_value = dgx_vbit_write_value;
    scr->wait_screen_buffer = dgx_vbit_wait_data;
    scr->fill_hline = dgx_vbit_fill_hline;
    scr->fill_vline = dgx_vbit_fill_vline;
    scr->fast_hline = dgx_vbit_fast_hline;
    scr->fast_vline = dgx_vbit_fast_vline;
    scr->fill_rectangle = dgx_vbit_fill_rectangle;
    scr->update_screen = dgx_vbit_update_screen;
    scr->destroy = dgx_vbit_destroy_screen;
    return ESP_OK;
}

