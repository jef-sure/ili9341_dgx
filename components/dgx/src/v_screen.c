#include <string.h>

#include "esp_heap_caps.h"
#include "dgx_v_screen.h"

void dgx_v_set_area(struct dgx_screen_ *scr, uint16_t caset_lo, uint16_t caset_hi, uint16_t raset_lo, uint16_t raset_hi) {
    scr->fb_col_lo = caset_lo;
    scr->fb_col_hi = caset_hi;
    scr->fb_row_lo = raset_lo;
    scr->fb_row_hi = raset_hi;
    scr->fb_pointer_x = caset_lo;
    scr->fb_pointer_y = raset_lo;
}

void dgx_v_write_data(dgx_screen_t *scr, uint8_t *data, uint32_t lenbits) {
    uint32_t xyoffset = (uint32_t) scr->fb_pointer_y * scr->width + scr->fb_pointer_x;
    uint8_t *pb = scr->frame_buffer + scr->lcd_ram_bits * xyoffset / 8;
    uint32_t didx = 0;
    while (lenbits >= scr->lcd_ram_bits) {
        if (scr->fb_pointer_x > scr->fb_col_hi) {
            scr->fb_pointer_x = scr->fb_col_lo;
            scr->fb_pointer_y++;
            xyoffset = (uint32_t) scr->fb_pointer_y * scr->width + scr->fb_pointer_x;
            pb = scr->frame_buffer + scr->lcd_ram_bits * xyoffset / 8;
        }
        if (scr->fb_pointer_y > scr->fb_row_hi) {
            scr->fb_pointer_y = scr->fb_row_lo;
            xyoffset = (uint32_t) scr->fb_pointer_y * scr->width + scr->fb_pointer_x;
            pb = scr->frame_buffer + scr->lcd_ram_bits * xyoffset / 8;
        }
        uint32_t value = dgx_read_buf_value(&data, didx, scr->lcd_ram_bits);
        pb = dgx_fill_buf_value(pb, xyoffset, value, scr->lcd_ram_bits);
        didx++;
        xyoffset++;
        scr->fb_pointer_x++;
        lenbits -= scr->lcd_ram_bits;
    }
}

void dgx_v_write_data8(dgx_screen_t *scr, uint8_t *data, uint32_t lenbits) {
    uint32_t xyoffset = (uint32_t) scr->fb_pointer_y * scr->width + scr->fb_pointer_x;
    uint8_t *pb = scr->frame_buffer + xyoffset;
    while (lenbits != 0) {
        if (scr->fb_pointer_x > scr->fb_col_hi) {
            scr->fb_pointer_x = scr->fb_col_lo;
            scr->fb_pointer_y++;
            xyoffset = (uint32_t) scr->fb_pointer_y * scr->width + scr->fb_pointer_x;
            pb = scr->frame_buffer + xyoffset;
        }
        if (scr->fb_pointer_y > scr->fb_row_hi) {
            scr->fb_pointer_y = scr->fb_row_lo;
            xyoffset = (uint32_t) scr->fb_pointer_y * scr->width + scr->fb_pointer_x;
            pb = scr->frame_buffer + xyoffset;
        }
        *pb++ = *data++;
        scr->fb_pointer_x++;
        lenbits -= 8;
    }
}

void dgx_v_write_value(dgx_screen_t *scr, uint32_t value) {
    if (scr->fb_pointer_x >= scr->fb_col_hi) {
        scr->fb_pointer_x = scr->fb_col_lo;
        scr->fb_pointer_y++;
    }
    if (scr->fb_pointer_y >= scr->fb_row_hi) {
        scr->fb_pointer_y = scr->fb_row_lo;
    }
    scr->draw_pixel(scr, scr->fb_pointer_x++, scr->fb_pointer_y, value);
}

void dgx_v_draw_pixel(dgx_screen_t *scr, int16_t x, int16_t y, uint32_t color) {
    if (x < 0 || y < 0 || x >= scr->width || y >= scr->height) return;
    uint32_t xyoffset = (uint32_t) y * scr->width + x;
    uint8_t *pb = scr->frame_buffer + scr->lcd_ram_bits * xyoffset / 8;
    dgx_fill_buf_value(pb, xyoffset, color, scr->lcd_ram_bits);
    if (!scr->in_progress) scr->update_screen(scr, x, y, x, y);
}

void dgx_v_draw_pixel8(dgx_screen_t *scr, int16_t x, int16_t y, uint32_t color) {
    if (x < 0 || y < 0 || x >= scr->width || y >= scr->height) return;
    uint32_t xyoffset = (uint32_t) y * scr->width + x;
    uint8_t *pb = scr->frame_buffer + xyoffset;
    *pb = (uint8_t) color;
    if (!scr->in_progress) scr->update_screen(scr, x, y, x, y);
}

uint32_t dgx_v_read_pixel(dgx_screen_t *scr, int16_t x, int16_t y) {
    if (x < 0 || y < 0 || x >= scr->width || y >= scr->height) return 0;
    uint32_t xyoffset = (uint32_t) y * scr->width + x;
    uint8_t *pb = scr->frame_buffer + scr->lcd_ram_bits * xyoffset / 8;
    return dgx_read_buf_value(&pb, xyoffset, scr->lcd_ram_bits);
}

uint32_t dgx_v_read_pixel8(dgx_screen_t *scr, int16_t x, int16_t y) {
    if (x < 0 || y < 0 || x >= scr->width || y >= scr->height) return 0;
    uint32_t xyoffset = (uint32_t) y * scr->width + x;
    uint8_t *pb = scr->frame_buffer + xyoffset;
    return *pb;
}

void dgx_v_wait_data(dgx_screen_t *scr) {
    return;
}

void dgx_v_fill_hline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, uint32_t color, uint32_t bg, uint32_t mask, uint8_t mask_bits) {
    if (y < 0 || y >= scr->height) return;
    const uint32_t test_bit = 1;
    scr->in_progress++;
    for (uint16_t i = 0; i < w; ++i, mask = ror_bits(mask, mask_bits)) {
        uint32_t c = mask & test_bit ? color : bg;
        dgx_v_draw_pixel(scr, x + i, y, c);
    }
    if (!--scr->in_progress) scr->update_screen(scr, x, y, x + w - 1, y);
}

void dgx_v_fill_vline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t h, uint32_t color, uint32_t bg, uint32_t mask, uint8_t mask_bits) {
    if (x < 0 || x >= scr->width) return;
    const uint32_t test_bit = 1;
    scr->in_progress++;
    for (uint16_t i = 0; i < h; ++i, mask = ror_bits(mask, mask_bits)) {
        uint32_t c = mask & test_bit ? color : bg;
        dgx_v_draw_pixel(scr, x, y + i, c);
    }
    if (!--scr->in_progress) scr->update_screen(scr, x, y, x, y + h - 1);
}

void dgx_v_fast_vline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t h, uint32_t color) {
    if (x < 0 || x >= scr->width) return;
    scr->in_progress++;
    for (uint16_t i = 0; i < h; ++i) {
        dgx_v_draw_pixel(scr, x, y + i, color);
    }
    if (!--scr->in_progress) scr->update_screen(scr, x, y, x, y + h - 1);
}

void dgx_v_fast_vline8(dgx_screen_t *scr, int16_t x, int16_t y, int16_t h, uint32_t color) {
    if (x < 0 || x >= scr->width) return;
    if (y < 0) {
        h = y + h;
        y = 0;
    }
    if (y + h > scr->height) {
        h = scr->height - y;
    }
    if (h <= 0) return;
    uint32_t xyoffset = (uint32_t) y * scr->width + x;
    uint8_t *pb = scr->frame_buffer;
    for (uint16_t i = 0; i < h; ++i, xyoffset += scr->width) {
        pb[xyoffset] = (uint8_t) color;
    }
    if (!scr->in_progress) scr->update_screen(scr, x, y, x, y + h - 1);
}

void dgx_v_fast_hline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, uint32_t color) {
    if (y < 0 || y >= scr->height) return;
    scr->in_progress++;
    for (uint16_t i = 0; i < w; ++i) {
        dgx_v_draw_pixel(scr, x + i, y, color);
    }
    if (!--scr->in_progress) scr->update_screen(scr, x, y, x + w - 1, y);
}

void dgx_v_fast_hline8(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, uint32_t color) {
    if (y < 0 || y >= scr->height) return;
    if (x < 0) {
        w = x + w;
        x = 0;
    }
    if (x + w > scr->width) {
        w = scr->width - x;
    }
    if (w <= 0) return;
    uint32_t xyoffset = (uint32_t) y * scr->width + x;
    uint8_t *pb = scr->frame_buffer + xyoffset;
    for (uint16_t i = 0; i < w; ++i) {
        pb[i] = (uint8_t) color;
    }
    if (!scr->in_progress) scr->update_screen(scr, x, y, x + w - 1, y);
}

void dgx_v_fill_rectangle(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color) {
    if (w < 0 || x + w < 0 || x >= scr->width || h < 0 || y + h < 0 || y >= scr->height) return;
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
    if (h <= 0 || w <= 0) return;
    scr->in_progress++;
    for (uint16_t i = 0; i < h; ++i) {
        dgx_v_fast_hline(scr, x, y + i, w, color);
    }
    if (!--scr->in_progress) scr->update_screen(scr, x, y, x + w - 1, y + h - 1);
}

void dgx_v_fill_rectangle8(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color) {
    if (w < 0 || x + w < 0 || x >= scr->width || h < 0 || y + h < 0 || y >= scr->height) return;
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
    if (h <= 0 || w <= 0) return;
    for (uint16_t i = 0; i < h; ++i) {
        uint32_t xyoffset = (uint32_t) (y + i) * scr->width + x;
        uint8_t *pb = scr->frame_buffer + xyoffset;
        for (uint16_t i = 0; i < w; ++i) {
            pb[i] = (uint8_t) color;
        }
    }
    if (!scr->in_progress) scr->update_screen(scr, x, y, x + w - 1, y + h - 1);
}

void dgx_v_update_screen(dgx_screen_t *scr, int16_t x1, int16_t y1, int16_t x2, int16_t y2) {

}

void dgx_v_destroy_screen(dgx_screen_t *scr) {
    free(scr->draw_buffer);
    free(scr->frame_buffer);
}

esp_err_t dgx_v_init(dgx_screen_t *scr, int16_t width, int16_t height, uint8_t color_bits) {
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
    scr->color_bits = color_bits;
    scr->lcd_ram_bits = color_bits == 12 ? 12 : 8 * ((color_bits + 7) / 8);
    scr->dir_x = LeftRight;
    scr->dir_y = TopBottom;
    scr->swap_xy = false;
    scr->draw_buffer_len = 4088;
    scr->draw_buffer = heap_caps_calloc(1, scr->draw_buffer_len, MALLOC_CAP_DMA);
    if (!scr->draw_buffer) return ESP_FAIL;
    uint32_t framebuf_size = ((uint32_t) scr->width * scr->height * scr->lcd_ram_bits + 7) / 8;
    scr->frame_buffer = heap_caps_calloc(1, framebuf_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!scr->frame_buffer) {
        free(scr->draw_buffer);
        scr->draw_buffer = 0;
        return ESP_FAIL;
    }
    dgx_v_set_area(scr, 0, width - 1, 0, height - 1);
    scr->set_area = dgx_v_set_area;
    scr->update_screen = dgx_v_update_screen;
    scr->destroy = dgx_v_destroy_screen;
    scr->wait_screen_buffer = dgx_v_wait_data;
    scr->write_value = dgx_v_write_value;
    scr->fill_hline = dgx_v_fill_hline;
    scr->fill_vline = dgx_v_fill_vline;
    if (color_bits == 8) {
        scr->draw_pixel = dgx_v_draw_pixel8;
        scr->read_pixel = dgx_v_read_pixel8;
        scr->write_area = dgx_v_write_data8;
        scr->fast_hline = dgx_v_fast_hline8;
        scr->fast_vline = dgx_v_fast_vline8;
        scr->fill_rectangle = dgx_v_fill_rectangle8;
    } else {
        scr->draw_pixel = dgx_v_draw_pixel;
        scr->read_pixel = dgx_v_read_pixel;
        scr->write_area = dgx_v_write_data;
        scr->fast_hline = dgx_v_fast_hline;
        scr->fast_vline = dgx_v_fast_vline;
        scr->fill_rectangle = dgx_v_fill_rectangle;
    }
    return ESP_OK;
}

