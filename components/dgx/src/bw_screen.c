#include <string.h>

#include "esp_heap_caps.h"
#include "dgx_bw_screen.h"

void dgx_bw_set_area(struct dgx_screen_ *scr, uint16_t caset_lo, uint16_t caset_hi, uint16_t raset_lo,
        uint16_t raset_hi) {
    scr->fb_col_lo = caset_lo;
    scr->fb_col_hi = caset_hi;
    scr->fb_row_lo = raset_lo;
    scr->fb_row_hi = raset_hi;
    scr->fb_pointer_x = caset_lo;
    scr->fb_pointer_y = raset_lo;
}

void dgx_bw_write_data(dgx_screen_t *scr, uint8_t *data, uint32_t lenbits) {
    uint8_t b = *data;
    uint8_t mask = 0x80;
    uint8_t page = scr->fb_pointer_y >> 3;
    uint8_t bit = scr->fb_pointer_y & 7;
    uint8_t rmask = 1 << bit;
    uint8_t *pb = scr->frame_buffer + page * scr->width + scr->fb_pointer_x;
    while (lenbits) {
        if (scr->fb_pointer_x > scr->fb_col_hi) {
            scr->fb_pointer_x = scr->fb_col_lo;
            scr->fb_pointer_y++;
            page = scr->fb_pointer_y >> 3;
            bit = scr->fb_pointer_y & 7;
            rmask = 1 << bit;
            pb = scr->frame_buffer + page * scr->width + scr->fb_pointer_x;
        }
        if (scr->fb_pointer_y > scr->fb_row_hi) {
            scr->fb_pointer_y = scr->fb_row_lo;
            page = scr->fb_pointer_y >> 3;
            bit = scr->fb_pointer_y & 7;
            rmask = 1 << bit;
            pb = scr->frame_buffer + page * scr->width + scr->fb_pointer_x;
        }
        if (!mask) {
            b = *++data;
            mask = 0x80;
        }
        if (mask & b)
            *pb |= rmask;
        else
            *pb &= ~rmask;
        pb++;
        scr->fb_pointer_x++;
        lenbits--;
        mask >>= 1;
    }
}

void dgx_bw_write_value(dgx_screen_t *scr, uint32_t value) {
    if (scr->fb_pointer_x >= scr->fb_col_hi) {
        scr->fb_pointer_x = scr->fb_col_lo;
        scr->fb_pointer_y++;
    }
    if (scr->fb_pointer_y >= scr->fb_row_hi) {
        scr->fb_pointer_y = scr->fb_row_lo;
    }
    scr->draw_pixel(scr, scr->fb_pointer_x, scr->fb_pointer_y, value);
}

void dgx_bw_draw_pixel(dgx_screen_t *scr, int16_t x, int16_t y, uint32_t color) {
    uint8_t page = (uint16_t) y >> 3;
    uint8_t bit = y & 7;
    uint8_t mask = 1 << bit;
    uint8_t *pb = scr->frame_buffer + page * scr->width + x;
    if (color)
        *pb |= mask;
    else
        *pb &= ~mask;
    if (!scr->in_progress) scr->update_screen(scr, x, y, x, y);
}

uint32_t dgx_bw_read_pixel(dgx_screen_t *scr, int16_t x, int16_t y) {
    uint8_t page = (uint16_t) y >> 3;
    uint8_t bit = y & 7;
    uint8_t mask = 1 << bit;
    uint8_t *pb = scr->frame_buffer + page * scr->width + x;
    return !!(*pb & mask);
}

void dgx_bw_wait_data(dgx_screen_t *scr) {
    return;
}

void dgx_bw_fill_hline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, uint32_t color, uint32_t bg, uint32_t mask,
        uint8_t mask_bits) {
    const uint32_t test_bit = 1;
    scr->in_progress++;
    for (uint16_t i = 0; i < w; ++i, mask = ror_bits(mask, mask_bits)) {
        uint32_t c = mask & test_bit ? color : bg;
        dgx_bw_draw_pixel(scr, x + i, y, c);
    }
    if (!--scr->in_progress) scr->update_screen(scr, x, y, x + w - 1, y);
}

void dgx_bw_fill_vline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t h, uint32_t color, uint32_t bg, uint32_t mask,
        uint8_t mask_bits) {
    const uint32_t test_bit = 1;
    scr->in_progress++;
    for (uint16_t i = 0; i < h; ++i, mask = ror_bits(mask, mask_bits)) {
        uint32_t c = mask & test_bit ? color : bg;
        dgx_bw_draw_pixel(scr, x, y + i, c);
    }
    if (!--scr->in_progress) scr->update_screen(scr, x, y, x, y + h - 1);
}

void dgx_bw_fast_vline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t h, uint32_t color) {
    int16_t y2 = y + h - 1;
    uint8_t page = (uint16_t) y >> 3;
    uint8_t *pb = scr->frame_buffer + page * scr->width + x;
    for (int16_t yl = y; yl <= y2; yl += 8 - (yl & 7), pb += scr->width) {
        page = (uint16_t) yl >> 3;
        uint8_t mask = 0xff << (yl & 7);
        if ((yl & ~7) == (y2 & ~7)) mask &= 0xff >> (7 - (y2 & 7));
        if (color)
            *pb |= mask;
        else
            *pb &= ~mask;
    }
    if (!scr->in_progress) scr->update_screen(scr, x, y, x, y + h - 1);
}

void dgx_bw_fast_hline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, uint32_t color) {
    uint8_t page = (uint16_t) y >> 3;
    uint8_t *pb = scr->frame_buffer + page * scr->width + x;
    uint8_t *pe = pb + w;
    uint8_t mask = 1 << (y & 7);
    if (color)
        while (pb != pe)
            *pb++ |= mask;
    else {
        mask = ~mask;
        while (pb != pe)
            *pb++ &= mask;
    }
    if (!scr->in_progress) scr->update_screen(scr, x, y, x + w - 1, y);
}

void dgx_bw_fill_rectangle(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color) {
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
    for (uint16_t i = 0; i < w; ++i) {
        dgx_bw_fast_vline(scr, x + i, y, h, color);
    }
    if (!--scr->in_progress) scr->update_screen(scr, x, y, x + w - 1, y + h - 1);
}

void dgx_bw_update_screen(dgx_screen_t *scr, int16_t x1, int16_t y1, int16_t x2, int16_t y2) {

}

void dgx_bw_destroy_screen(dgx_screen_t *scr) {
    free(scr->draw_buffer);
    free(scr->frame_buffer);
}

esp_err_t dgx_bw_init(dgx_screen_t *scr, int16_t width, int16_t height) {
    scr->pending_transactions = 0;
    scr->max_transactions = 20;
    scr->is_data_free = true;
    scr->in_progress = 0;
    scr->width = width;
    scr->height = height;
    scr->color_bits = 1;
    scr->lcd_ram_bits = 1;
    scr->dir_x = LeftRight;
    scr->dir_y = TopBottom;
    scr->swap_xy = false;
    scr->draw_buffer_len = 4088;
    scr->draw_buffer = heap_caps_calloc(1, scr->draw_buffer_len, MALLOC_CAP_DMA);
    if (!scr->draw_buffer) return ESP_FAIL;
    scr->frame_buffer = heap_caps_calloc(1, (scr->width * scr->height + 7) / 8, MALLOC_CAP_DMA);
    if (!scr->frame_buffer) {
        free(scr->draw_buffer);
        return ESP_FAIL;
    }
    dgx_bw_set_area(scr, 0, width - 1, 0, height - 1);
    scr->set_area = dgx_bw_set_area;
    scr->draw_pixel = dgx_bw_draw_pixel;
    scr->read_pixel = dgx_bw_read_pixel;
    scr->write_area = dgx_bw_write_data;
    scr->write_value = dgx_bw_write_value;
    scr->wait_screen_buffer = dgx_bw_wait_data;
    scr->fill_hline = dgx_bw_fill_hline;
    scr->fill_vline = dgx_bw_fill_vline;
    scr->fast_hline = dgx_bw_fast_hline;
    scr->fast_vline = dgx_bw_fast_vline;
    scr->fill_rectangle = dgx_bw_fill_rectangle;
    scr->update_screen = dgx_bw_update_screen;
    scr->destroy = dgx_bw_destroy_screen;
    return ESP_OK;
}

