#include <string.h>
#include <stdbool.h>
#include "dgx_bitmap.h"

dgx_bw_bitmap_t dgx_bw_bitmap_allocate(int16_t width, int16_t height) {
    dgx_bw_bitmap_t ret = { 0, width, height };
    ret.bitmap = calloc(1, ((width + 7) / 8) * height);
    return ret;
}

void dgx_bw_bitmap_clear(dgx_bw_bitmap_t *bmap, bool invert) {
    uint8_t val = invert ? 0xff : 0;
    memset(bmap->bitmap, val, ((bmap->width + 7) / 8) * bmap->height);
}

void dgx_bw_bitmap_free(dgx_bw_bitmap_t *bmap) {
    free(bmap->bitmap);
}

/*
 * idx = 0 .. N
 * len = 1 .. 8
 *
 */

static void dgx_set_bits8(uint8_t *array, size_t idx, uint8_t msb_value, uint8_t len) {
    size_t ai = idx >> 3;
    size_t bi = idx & 7;
    size_t aei = (idx + len - 1) >> 3;
    if (ai == aei) {
        if (bi == 0 && len == 8) {
            array[ai] = msb_value;
        } else {
            msb_value >>= bi;
            uint8_t mask = ((uint8_t) 0xff << (8 - len - bi)) & ((uint8_t) 0xff >> bi);
            array[ai] &= ~mask;
            array[ai] |= msb_value & mask;
        }
    } else {
        uint8_t evalue = msb_value >> bi;
        uint8_t mask = (uint8_t) 0xff << (8 - bi);
        array[ai] &= mask;
        array[ai] |= evalue & ~mask;
        evalue = msb_value << (8 - bi);
        size_t bei = (idx + len) & 7;
        mask = (uint8_t) 0xff >> bei;
        array[aei] &= mask;
        array[aei] |= evalue & ~mask;
    }
}

void dgx_bw_bitmap_set_pixel(dgx_bw_bitmap_t *bmap, int16_t x, int16_t y, uint8_t value) {
    uint16_t pitch = (bmap->width + 7) / 8;
    uint8_t *bc = bmap->bitmap + y * pitch + x / 8;
    uint8_t bmask = 0x80 >> (x & 7);
    if (value)
        *bc |= bmask;
    else
        *bc &= ~bmask;
}

bool dgx_bw_bitmap_get_pixel(dgx_bw_bitmap_t *bmap, int16_t x, int16_t y) {
    uint16_t pitch = (bmap->width + 7) / 8;
    uint8_t *bc = bmap->bitmap + y * pitch + x / 8;
    uint8_t bmask = 0x80 >> (x & 7);
    return !!(*bc & bmask);
}

void dgx_bw_bitmap_to_screen(dgx_screen_t *scr, dgx_bw_bitmap_t *bmap, int16_t x, int16_t y, uint32_t color,
        uint32_t bg_color, dgx_orientation_t xdir, dgx_orientation_t ydir, bool swap_xy) {
    int16_t x1 = x;
    int16_t y1 = y;
    int16_t skip_x = 0;
    int16_t skip_y = 0;
    int16_t bw;
    int16_t bh;
    if (swap_xy) {
        bw = bmap->height;
        bh = bmap->width;
    } else {
        bw = bmap->width;
        bh = bmap->height;
    }
    if (x1 < 0) {
        if (xdir == LeftRight) skip_x = -x1;
        bw += x1;
        x1 = 0;
    }
    if (y1 < 0) {
        if (ydir == TopBottom) skip_y = -y1;
        bh += y1;
        y1 = 0;
    }
    if (x1 + bw > scr->width) {
        if (xdir == RightLeft) skip_x = x1 + bw - scr->width;
        bw = scr->width - x1;
    }
    if (y1 + bh > scr->height) {
        if (ydir == BottomTop) skip_y = y1 + bh - scr->height;
        bh = scr->height - y1;
    }
    if (bh <= 0 || bw <= 0) return;
    ++scr->in_progress;
    scr->set_area(scr, x1, x1 + bw - 1, y1, y1 + bh - 1);
    scr->wait_screen_buffer(scr);
    uint16_t lbidx = 0;
    uint8_t *cbuf = scr->draw_buffer;
    if (swap_xy) {
        for (int16_t br = 0; br < bh; ++br) {
            int16_t row = ydir == BottomTop ? bmap->height - 1 - br : br;
            for (int16_t bc = 0; bc < bw; ++bc) {
                int16_t col = xdir == RightLeft ? bmap->width - 1 - bc : bc;
                bool px = dgx_bw_bitmap_get_pixel(bmap, row + skip_y, col + skip_x);
                uint32_t c = px ? color : bg_color;
                cbuf = dgx_fill_buf_value(cbuf, lbidx++, c, scr->lcd_ram_bits);
                if (cbuf - scr->draw_buffer >= scr->draw_buffer_len - 32 && (lbidx & 7) == 0) {
                    scr->write_area(scr, scr->draw_buffer, scr->lcd_ram_bits * (uint32_t) lbidx);
                    scr->wait_screen_buffer(scr);
                    lbidx = 0;
                    cbuf = scr->draw_buffer;
                }
            }
        }
    } else {
        for (int16_t br = 0; br < bh; ++br) {
            int16_t row = ydir == BottomTop ? bmap->height - 1 - br : br;
            for (int16_t bc = 0; bc < bw; ++bc) {
                int16_t col = xdir == RightLeft ? bmap->width - 1 - bc : bc;
                bool px = dgx_bw_bitmap_get_pixel(bmap, col + skip_x, row + skip_y);
                uint32_t c = px ? color : bg_color;
                cbuf = dgx_fill_buf_value(cbuf, lbidx++, c, scr->lcd_ram_bits);
                if (cbuf - scr->draw_buffer >= scr->draw_buffer_len - 32 && (lbidx & 7) == 0) {
                    scr->write_area(scr, scr->draw_buffer, scr->lcd_ram_bits * (uint32_t) lbidx);
                    scr->wait_screen_buffer(scr);
                    lbidx = 0;
                    cbuf = scr->draw_buffer;
                }
            }
        }
    }
    if (lbidx) {
        scr->write_area(scr, scr->draw_buffer, scr->lcd_ram_bits * (uint32_t) lbidx);
        scr->wait_screen_buffer(scr);
    }
    if (!--scr->in_progress) scr->update_screen(scr, x1, y1, x1 + bw - 1, y1 + bh - 1);
}

void dgx_bw_bitmap_to_screen_transp(dgx_screen_t *scr, dgx_bw_bitmap_t *bmap, int16_t x, int16_t y, uint32_t color,
        dgx_orientation_t xdir, dgx_orientation_t ydir, bool swap_xy) {
    int16_t x1, y1, bw, bh;
    int16_t skip_x = 0, skip_y = 0;
    if (swap_xy) {
        bw = bmap->height;
        bh = bmap->width;
    } else {
        bw = bmap->width;
        bh = bmap->height;
    }
    x1 = x;
    y1 = y;
//    if (xdir == RightLeft) x1 -= bw - 1;
//    if (ydir == BottomTop) y1 -= bh - 1;
    if (x1 < 0) {
        if (xdir == LeftRight) skip_x = -x1;
        bw += x1;
        x1 = 0;
    }
    if (y1 < 0) {
        if (ydir == BottomTop) skip_y = -y1;
        bh += y1;
        y1 = 0;
    }
    if (x1 + bw > scr->width) {
        if (xdir == RightLeft) skip_x = x1 + bw - scr->width;
        bw = scr->width - x1;
    }
    if (y1 + bh > scr->height) {
        if (ydir == BottomTop) skip_y = y1 + bh - scr->height;
        bh = scr->height - y1;
    }
    if (bh <= 0 || bw <= 0) return;
    ++scr->in_progress;
    scr->set_area(scr, x1, x1 + bw - 1, y1, y1 + bh - 1);
    if (swap_xy) {
        for (int16_t br = 0; br < bh; ++br) {
            int16_t row = ydir == BottomTop ? bmap->height - 1 - br : br;
            for (int16_t bc = 0; bc < bw; ++bc) {
                int16_t col = xdir == RightLeft ? bmap->width - 1 - bc : bc;
                bool px = dgx_bw_bitmap_get_pixel(bmap, row + skip_y, col + skip_x);
                if (px) scr->draw_pixel(scr, col + x1, br + y1, color);
            }
        }
    } else {
        for (int16_t br = 0; br < bh; ++br) {
            int16_t row = ydir == BottomTop ? bmap->height - 1 - br : br;
            for (int16_t bc = 0; bc < bw; ++bc) {
                int16_t col = xdir == RightLeft ? bmap->width - 1 - bc : bc;
                bool px = dgx_bw_bitmap_get_pixel(bmap, col + skip_x, row + skip_y);
                if (px) scr->draw_pixel(scr, col + x1, br + y1, color);
            }
        }
    }
    if (!--scr->in_progress) scr->update_screen(scr, x1, y1, x1 + bw - 1, y1 + bh - 1);
}

void dgx_bw_bitmap_set8_bits_(dgx_bw_bitmap_t *bmap_dst, int16_t x, int16_t y, uint8_t val) {
    uint16_t dst_pitch = (bmap_dst->width + 7) / 8;
    uint8_t *dst_line = bmap_dst->bitmap + y * dst_pitch;
    dgx_set_bits8(dst_line, 0, val, 8);
}

void dgx_bw_bitmap_to_bitmap(dgx_bw_bitmap_t *bmap_dst, int16_t x, int16_t y, dgx_bw_bitmap_t *bmap_src) {
    int16_t x1, y1, bw, bh;
    int16_t skip_src_x = 0, skip_src_y = 0;
    bw = bmap_src->width;
    bh = bmap_src->height;
    x1 = x;
    y1 = y;
    if (x1 < 0) {
        skip_src_x = -x1;
        bw += x1;
        x1 = 0;
    }
    if (y1 < 0) {
        skip_src_y = -y1;
        bw += y1;
        y1 = 0;
    }
    if (x1 + bw > bmap_dst->width) {
        bw = bmap_dst->width - x1;
    }
    if (y1 + bh > bmap_dst->height) {
        bh = bmap_dst->height - y1;
    }
    if (bw <= 0 || bh <= 0) return;
    uint16_t dst_pitch = (bmap_dst->width + 7) / 8;
    uint16_t src_pitch = (bmap_src->width + 7) / 8;
    uint8_t *dst_line = bmap_dst->bitmap + y1 * dst_pitch;
    uint8_t *src_line = bmap_src->bitmap + skip_src_y * src_pitch;
    for (int16_t r = 0; r < bh; ++r, dst_line += dst_pitch, src_line += src_pitch) {
        for (int16_t c = 0; c < bw;) {
            uint8_t val = src_line[(uint16_t) (c + skip_src_x) >> 3];
            uint8_t ri = (c + skip_src_x) & 7;
            uint8_t blen = 8 - ri;
            if (c + blen > bw) blen = bw - c;
            if (ri) val <<= ri;
            dgx_set_bits8(dst_line, x1 + c, val, blen);
            c += blen;
        }
    }
}

