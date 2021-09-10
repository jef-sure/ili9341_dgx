#include "dgx_font.h"

uint32_t decodeUTF8next(const char *chr, size_t *idx) {
    uint32_t c = chr[*idx];
    if (c < 0x80) {
        if (c) ++*idx;
        return c;
    }
    ++*idx;
    uint8_t len;
    if ((c & 0xE0) == 0xC0) {
        c &= 0x1f;
        len = 2;
    } else if ((c & 0xF0) == 0xE0) {
        c &= 0xf;
        len = 3;
    } else if ((c & 0xF8) == 0xF0) {
        c &= 0x7;
        len = 4;
    } else {
        return c;
    }
    while (--len) {
        uint32_t nc = chr[(*idx)++];
        if ((nc & 0xC0) == 0x80) {
            c <<= 6;
            c |= nc & 0x3f;
        } else {
            break;
        }
    }
    return c;
}

const glyph_t* dgx_font_find_glyph(uint32_t codePoint, dgx_font_t *font, int16_t *xAdvance) {
    const glyph_t *g = 0, *fg = 0;
    for (const glyph_array_t *r = font->glyph_ranges; r->first; ++r) {
        if (!fg) fg = r->glyphs;
        if (codePoint >= r->first && codePoint < r->first + r->number) {
            g = r->glyphs + (codePoint - r->first);
            break;
        }
    }
    if (g == 0 && fg) {
        *xAdvance = fg->xAdvance;
        return 0;
    }
    if (g == 0 && fg == 0) {
        *xAdvance = 0;
        return 0;
    }
    *xAdvance = g->xAdvance;
    return g;
}

int16_t dgx_font_char_to_screen(dgx_screen_t *scr, int16_t x, int16_t y, uint32_t codePoint, uint32_t color,
        uint32_t bg_color, dgx_orientation_t xdir, dgx_orientation_t ydir, bool swap_xy, dgx_font_t *font) {
    int16_t xAdvance;
    const glyph_t *g = dgx_font_find_glyph(codePoint, font, &xAdvance);
    if (!g) return xAdvance;
    dgx_bw_bitmap_t bmap = { .bitmap = (uint8_t*) g->bitmap, .width = g->width, .height = g->height };
    if (bg_color == color) {
        dgx_bw_bitmap_to_screen_transp(scr, &bmap, x + g->xOffset, y + g->yOffset, color, xdir, ydir, swap_xy);
    } else {
        dgx_bw_bitmap_to_screen(scr, &bmap, x + g->xOffset, y + g->yOffset, color, bg_color, xdir, ydir, swap_xy);
    }
    return xAdvance;
}

int16_t dgx_font_char_to_bw_bitmap(dgx_bw_bitmap_t *bmap, int16_t x, int16_t y, uint32_t codePoint, dgx_font_t *font) {
    int16_t xAdvance;
    const glyph_t *g = dgx_font_find_glyph(codePoint, font, &xAdvance);
    if (!g) return xAdvance;
    dgx_bw_bitmap_t bmap_src = { .bitmap = (uint8_t*) g->bitmap, .width = g->width, .height = g->height };
    dgx_bw_bitmap_to_bitmap(bmap, x, y, &bmap_src);
    return g->xAdvance;
}

int16_t dgx_font_string_bounds(const char *str, dgx_font_t *font, int16_t *ycorner, int16_t *height) {
    size_t idx = 0;
    int16_t width = 0, yb = 0, yt = 0;
    bool is_first = true;
    while (str[idx]) {
        uint32_t cp = decodeUTF8next(str, &idx);
        int16_t xAdvance;
        const glyph_t *g = dgx_font_find_glyph(cp, font, &xAdvance);
        width += xAdvance;
        if (g) {
            int16_t bl = g->yOffset;
            int16_t bh = g->height + g->yOffset;
            if (is_first || bl < yt) yt = bl;
            if (is_first || bh > yb) yb = bh;
            is_first = false;
        }
    }
    *ycorner = yt;
    *height = yb - yt;
    return width;
}

void dgx_font_string_utf8_screen(dgx_screen_t *scr, int16_t x, int16_t y, const char *str, uint32_t color,
        uint32_t bg_color, dgx_orientation_t xdir, dgx_orientation_t ydir, bool swap_xy, dgx_font_t *font) {
    size_t idx = 0;
    if (bg_color != color) {
        int16_t width = 0, height = 0, ycorner;
        width = dgx_font_string_bounds(str, font, &ycorner, &height);
        scr->fill_rectangle(scr, x, y + ycorner, width, height, bg_color);
    }
    idx = 0;
    while (str[idx]) {
        uint32_t cp = decodeUTF8next(str, &idx);
        int16_t offset = dgx_font_char_to_screen(scr, x, y, cp, color, bg_color, xdir, ydir, swap_xy, font);
        if (swap_xy) {
            y += offset * ydir;
        } else {
            x += offset * xdir;
        }
    }
}

void dgx_draw_string_utf8_bitmap(dgx_bw_bitmap_t *bmap, int16_t x, int16_t y, const char *str, dgx_font_t *font) {
    size_t idx = 0;
    while (str[idx]) {
        uint32_t cp = decodeUTF8next(str, &idx);
        x += dgx_font_char_to_bw_bitmap(bmap, x, y, cp, font);
    }
}

