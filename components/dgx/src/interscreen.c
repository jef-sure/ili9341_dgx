/*
 * MIT License
 *
 * Copyright (c) 2021 Anton Petrusevich
 *
 */

#include "dgx_interscreen.h"

#define PIX12TORGB(r,g,b, ptr) \
    do {\
        uint8_t *pptr = (uint8_t *)(ptr);\
        (r) = (0xF0 & *pptr) | 0x0F;\
        (g) = ((0x0F & *pptr) << 4) | 0x0F;\
        (b) = (0xF0 & pptr[1] ) | 0x0F;\
    } while(0)

#define PIX16TORGB(r,g,b, ptr) \
    do {\
        uint8_t *pptr = (uint8_t *)(ptr);\
        (r) = (0xF8 & *pptr) | 0x07;\
        (g) = ((0x07 & *pptr) << 5) | ((0xE0 & pptr[1]) >> 3) | 0x03 ;\
        (b) = ((0x37 & pptr[1] ) << 3) | 0x07;\
    } while(0)

#define PIX18TORGB(r,g,b, ptr) \
    do {\
        uint8_t *pptr = (uint8_t *)(ptr);\
        (r) = pptr[0] | 0x03;\
        (g) = pptr[1] | 0x03;\
        (b) = pptr[2] | 0x03;\
    } while(0)

#define PIX24TORGB(r,g,b, ptr) \
    do {\
        uint8_t *pptr = (uint8_t *)(ptr);\
        (r) = pptr[0];\
        (g) = pptr[1];\
        (b) = pptr[2];\
    } while(0)

#define COLOR12TORGB(r,g,b, color) \
    do {\
        uint16_t lclr = (uint16_t)(color);\
        (r) = (lclr >> 8) | 0x0F;\
        (g) = ((lclr >> 4) & 0xF0) | 0x0F;\
        (b) = (lclr & 0xF0) | 0x0F;\
    } while(0)

#define COLOR16TORGB(r,g,b, color) \
    do {\
        uint16_t lclr = (uint16_t)(color);\
        (r) = (lclr >> 8) | 0x07;\
        (g) = ((lclr >> 5) & 0xFC) | 0x03;\
        (b) = ((lclr << 3) & 0xF8 ) | 0x07;\
    } while(0)

#define COLOR18TORGB(r,g,b, color) \
    do {\
        uint32_t lclr = (uint32_t)(color);\
        (r) = ((lclr >> 16) & 0xFC) | 0x03;\
        (g) = ((lclr >> 8) & 0xFC) | 0x03;\
        (b) = (lclr & 0xFC) | 0x03;\
    } while(0)

#define COLOR24TORGB(r,g,b, color) \
    do {\
        uint32_t lclr = (uint32_t)(color);\
        (r) = (lclr >> 16) & 0xFF;\
        (g) = (lclr >> 8) & 0xFF;\
        (b) = lclr & 0xFF;\
    } while(0)

void dgx_vscreen_to_screen(dgx_screen_t *scr_dst, int16_t x_dst, int16_t y_dst, dgx_screen_t *scr_src) {
    int16_t x1 = x_dst;
    int16_t y1 = y_dst;
    int16_t bw = scr_src->width;
    int16_t bh = scr_src->height;
    int16_t skip_x = 0;
    int16_t skip_y = 0;
    if (x1 < 0) {
        skip_x = -x1;
        bw += x1;
        x1 = 0;
    }
    if (y1 < 0) {
        skip_y = -y1;
        bh += y1;
        y1 = 0;
    }
    if (x1 + bw > scr_dst->width) {
        bw = scr_dst->width - x1;
    }
    if (y1 + bh > scr_dst->height) {
        bh = scr_dst->height - y1;
    }
    if (bh <= 0 || bw <= 0) return;
    ++scr_dst->in_progress;
    scr_dst->set_area(scr_dst, x1, x1 + bw - 1, y1, y1 + bh - 1);
    scr_dst->wait_screen_buffer(scr_dst);
    uint16_t lbidx = 0;
    uint8_t *cbuf = scr_dst->draw_buffer;
    if (scr_dst->lcd_ram_bits == scr_src->lcd_ram_bits) {
        for (int16_t br = 0; br < bh; ++br) {
            for (int16_t bc = 0; bc < bw; ++bc) {
                uint32_t color = scr_src->read_pixel(scr_src, skip_x + bc, skip_y + br);
                cbuf = dgx_fill_buf_value(cbuf, lbidx++, color, scr_dst->lcd_ram_bits);
                if (cbuf - scr_dst->draw_buffer >= scr_dst->draw_buffer_len - 32 && (lbidx & 7) == 0) {
                    scr_dst->write_area(scr_dst, scr_dst->draw_buffer, scr_dst->lcd_ram_bits * (uint32_t) lbidx);
                    scr_dst->wait_screen_buffer(scr_dst);
                    lbidx = 0;
                    cbuf = scr_dst->draw_buffer;
                }
            }
        }
        if (lbidx) {
            scr_dst->write_area(scr_dst, scr_dst->draw_buffer, scr_dst->lcd_ram_bits * (uint32_t) lbidx);
            scr_dst->wait_screen_buffer(scr_dst);
        }
    } else {
        // will be supported in the future
    }
    if (!--scr_dst->in_progress) scr_dst->update_screen(scr_dst, x1, y1, x1 + bw - 1, y1 + bh - 1);
}

void dgx_vscreen_to_vscreen(dgx_screen_t *scr_dst, int16_t x_dst, int16_t y_dst, dgx_screen_t *scr_src, dgx_bitblt_rop_t rop) {
    int16_t x1 = x_dst;
    int16_t y1 = y_dst;
    int16_t bw = scr_src->width;
    int16_t bh = scr_src->height;
    int16_t skip_x = 0;
    int16_t skip_y = 0;
    if (x1 < 0) {
        skip_x = -x1;
        bw += x1;
        x1 = 0;
    }
    if (y1 < 0) {
        skip_y = -y1;
        bh += y1;
        y1 = 0;
    }
    if (x1 + bw > scr_dst->width) {
        bw = scr_dst->width - x1;
    }
    if (y1 + bh > scr_dst->height) {
        bh = scr_dst->height - y1;
    }
    if (bh <= 0 || bw <= 0) return;
    ++scr_dst->in_progress;
    scr_dst->set_area(scr_dst, x1, x1 + bw - 1, y1, y1 + bh - 1);
    scr_dst->wait_screen_buffer(scr_dst);
    uint16_t lbidx = 0;
    uint8_t *cbuf = scr_dst->draw_buffer;
    if (scr_dst->lcd_ram_bits == scr_src->lcd_ram_bits) {
        for (int16_t br = 0; br < bh; ++br) {
            for (int16_t bc = 0; bc < bw; ++bc) {
                uint32_t color = scr_src->read_pixel(scr_src, skip_x + bc, skip_y + br);
                if (rop != BitbltSet) {
                    uint32_t color2 = scr_dst->read_pixel(scr_src, skip_x + bc, skip_y + br);
                    switch (rop) {
                    case BitbltOr:
                        color |= color2;
                        break;
                    case BitbltXor:
                        color ^= color2;
                        break;
                    case BitbltAnd:
                        color &= color2;
                        break;
                    default:
                        break;
                    }
                }
                cbuf = dgx_fill_buf_value(cbuf, lbidx++, color, scr_dst->lcd_ram_bits);
                if (cbuf - scr_dst->draw_buffer >= scr_dst->draw_buffer_len - 32 && (lbidx & 7) == 0) {
                    scr_dst->write_area(scr_dst, scr_dst->draw_buffer, scr_dst->lcd_ram_bits * (uint32_t) lbidx);
                    scr_dst->wait_screen_buffer(scr_dst);
                    lbidx = 0;
                    cbuf = scr_dst->draw_buffer;
                }
            }
        }
        if (lbidx) {
            scr_dst->write_area(scr_dst, scr_dst->draw_buffer, scr_dst->lcd_ram_bits * (uint32_t) lbidx);
            scr_dst->wait_screen_buffer(scr_dst);
        }
    } else {
        // will be supported in the future
    }
    if (!--scr_dst->in_progress) scr_dst->update_screen(scr_dst, x1, y1, x1 + bw - 1, y1 + bh - 1);
}

void dgx_copy_region_from_vscreen(dgx_screen_t *scr_dst, int16_t x_dst, int16_t y_dst, dgx_screen_t *scr_src, int16_t x_src, int16_t y_src,
        int16_t width, int16_t height, uint32_t outer_color) {
    int16_t x1 = x_dst;
    int16_t y1 = y_dst;
    int16_t bw = width;
    int16_t bh = height;
    int16_t skip_x = 0;
    int16_t skip_y = 0;
    if (x1 < 0) {
        skip_x = -x1;
        bw += x1;
        x1 = 0;
    }
    if (y1 < 0) {
        skip_y = -y1;
        bh += y1;
        y1 = 0;
    }
    if (x1 + bw > scr_dst->width) {
        bw = scr_dst->width - x1;
    }
    if (y1 + bh > scr_dst->height) {
        bh = scr_dst->height - y1;
    }
    if (bh <= 0 || bw <= 0) return;
    x_src += skip_x;
    y_src += skip_y;
    ++scr_dst->in_progress;
    scr_dst->set_area(scr_dst, x1, x1 + bw - 1, y1, y1 + bh - 1);
    scr_dst->wait_screen_buffer(scr_dst);
    uint16_t lbidx = 0;
    uint8_t *cbuf = scr_dst->draw_buffer;
    if (scr_dst->lcd_ram_bits == scr_src->lcd_ram_bits) {
        for (int16_t br = 0; br < bh; ++br) {
            for (int16_t bc = 0; bc < bw; ++bc) {
                int16_t src_x = bc + x_src;
                int16_t src_y = br + y_src;
                uint32_t color = //
                        src_x < scr_src->width && src_x >= 0 && src_y < scr_src->height && src_y >= 0 //
                                ? scr_src->read_pixel(scr_src, src_x, src_y) : outer_color;
                cbuf = dgx_fill_buf_value(cbuf, lbidx++, color, scr_dst->lcd_ram_bits);
                if (cbuf - scr_dst->draw_buffer >= scr_dst->draw_buffer_len - 32 && (lbidx & 7) == 0) {
                    scr_dst->write_area(scr_dst, scr_dst->draw_buffer, scr_dst->lcd_ram_bits * (uint32_t) lbidx);
                    scr_dst->wait_screen_buffer(scr_dst);
                    lbidx = 0;
                    cbuf = scr_dst->draw_buffer;
                }
            }
        }
        if (lbidx) {
            scr_dst->write_area(scr_dst, scr_dst->draw_buffer, scr_dst->lcd_ram_bits * (uint32_t) lbidx);
            scr_dst->wait_screen_buffer(scr_dst);
        }
    } else {
        // will be supported in the future
    }
    if (!--scr_dst->in_progress) scr_dst->update_screen(scr_dst, x1, y1, x1 + bw - 1, y1 + bh - 1);
}

dgx_screen_t* dgx_new_vscreen_from_region(dgx_screen_t *scr_src, int16_t x, int16_t y, int16_t width, int16_t height) {
    dgx_screen_t *scr = calloc(1, sizeof(dgx_screen_t));
    if (!scr) return 0;
    esp_err_t rc = dgx_v_init(scr, width, height, scr_src->color_bits);
    if (rc != 0) {
        free(scr);
        return 0;
    }
    dgx_copy_region_from_vscreen(scr, 0, 0, scr_src, x, y, width, height, 0);
    return scr;
}

void dgx_vscreen8_to_screen16(dgx_screen_t *scr_dst, int16_t x_dst, int16_t y_dst, dgx_screen_t *scr_src, uint16_t *lut) {
    int16_t x1 = x_dst;
    int16_t y1 = y_dst;
    int16_t bw = scr_src->width;
    int16_t bh = scr_src->height;
    int16_t skip_x = 0;
    int16_t skip_y = 0;
    if (x1 < 0) {
        skip_x = -x1;
        bw += x1;
        x1 = 0;
    }
    if (y1 < 0) {
        skip_y = -y1;
        bh += y1;
        y1 = 0;
    }
    if (x1 + bw > scr_dst->width) {
        bw = scr_dst->width - x1;
    }
    if (y1 + bh > scr_dst->height) {
        bh = scr_dst->height - y1;
    }
    if (bh <= 0 || bw <= 0) return;
    ++scr_dst->in_progress;
    scr_dst->set_area(scr_dst, x1, x1 + bw - 1, y1, y1 + bh - 1);
    scr_dst->wait_screen_buffer(scr_dst);
    uint8_t *cbuf = scr_dst->draw_buffer;
    uint32_t sw = scr_src->width;
    for (int16_t br = 0; br < bh; ++br) {
        for (int16_t bc = 0; bc < bw; ++bc) {
            uint8_t cidx = scr_src->frame_buffer[skip_x + bc + (skip_y + br) * sw];
            uint16_t color = lut[cidx];
            *cbuf++ = (uint8_t) (color >> 8);
            *cbuf++ = (uint8_t) color;
            if (cbuf - scr_dst->draw_buffer >= scr_dst->draw_buffer_len - 4) {
                scr_dst->write_area(scr_dst, scr_dst->draw_buffer, 8 * (cbuf - scr_dst->draw_buffer));
                scr_dst->wait_screen_buffer(scr_dst);
                cbuf = scr_dst->draw_buffer;
            }
        }
    }
    if (cbuf != scr_dst->draw_buffer) {
        scr_dst->write_area(scr_dst, scr_dst->draw_buffer, 8 * (cbuf - scr_dst->draw_buffer));
        scr_dst->wait_screen_buffer(scr_dst);
    }
    if (!--scr_dst->in_progress) scr_dst->update_screen(scr_dst, x1, y1, x1 + bw - 1, y1 + bh - 1);
}

