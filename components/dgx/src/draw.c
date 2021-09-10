#include "dgx_draw.h"
#include <math.h>

#define MIN(a,b) ((a) < (b) ? a : b)
#define MAX(a,b) ((a) < (b) ? b : a)
#define SWAP(a,b) do { (a) ^= (b); (b) ^= (a); (a) ^= (b); } while(0)

void dgx_draw_line(dgx_screen_t *scr, int16_t x, int16_t y, int16_t x2, int16_t y2, uint32_t color) {
    int16_t dx;
    int16_t dy;
    dx = x2 - x;
    dy = y2 - y;
    scr->in_progress++;
    if (dx == 0 && dy == 0) {
        scr->draw_pixel(scr, x, y, color);
        if (!--scr->in_progress) scr->update_screen(scr, x, y, x, y);
        return;
    } else if (dx == 0) {
        if (dy < 0) {
            y = y2;
            dy = -dy;
        }
        scr->fast_vline(scr, x, y, dy + 1, color);
        if (!--scr->in_progress) scr->update_screen(scr, x, y, x2, y2);
        return;
    } else if (dy == 0) {
        if (dx < 0) {
            x = x2;
            dx = -dx;
        }
        scr->fast_hline(scr, x, y, dx + 1, color);
        if (!--scr->in_progress) scr->update_screen(scr, x, y, x2, y2);
        return;
    }
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    if (dx == dy) {
        dx = x2 - x > 0 ? 1 : -1;
        dy = y2 - y > 0 ? 1 : -1;
        for (int16_t x1 = x, y1 = y; x1 != x2 + dx; x1 += dx, y1 += dy) {
            scr->draw_pixel(scr, x1, y1, color);
        }
        if (!--scr->in_progress) scr->update_screen(scr, x, y, x2, y2);
        return;
    }
    int16_t err;
    if (dx > dy) {
        err = dx / 2;
        if (x > x2) {
            SWAP(x, x2);
            SWAP(y, y2);
        }
        int16_t sy = y2 > y ? 1 : -1;
        int16_t x1 = x;
        int16_t y1 = y;
        int16_t sp = x;
        do {
            err += dy;
            if (err >= dx || x1 == x2) {
                scr->fast_hline(scr, sp, y1, x1 - sp + 1, color);
                sp = x1 + 1;
                y1 += sy;
                err -= dx;
            }
        } while (x1++ != x2);
    } else {
        err = dy / 2;
        if (y > y2) {
            SWAP(x, x2);
            SWAP(y, y2);
        }
        int16_t sx = x2 > x ? 1 : -1;
        int16_t x1 = x;
        int16_t y1 = y;
        int16_t sp = y;
        do {
            err += dx;
            if (err >= dy || y1 == y2) {
                scr->fast_vline(scr, x1, sp, y1 - sp + 1, color);
                sp = y1 + 1;
                x1 += sx;
                err -= dy;
            }
        } while (y1++ != y2);
    }
    if (!--scr->in_progress) scr->update_screen(scr, x, y, x2, y2);
}

typedef int32_t fixed_t;

const float FixedScaleF = 65536.0f;
const int32_t FixedScaleI = 65536;

static inline fixed_t float2fixed(float f) {
    return f * FixedScaleF;
}

static inline fixed_t int2fixed(int16_t f) {
    return f * FixedScaleI;
}

static inline int16_t round_fixed(fixed_t f) {
    return (f + FixedScaleI / 2) / FixedScaleI;
}

void dgx_draw_line_float(dgx_screen_t *scr, float xs, float ys, float xe, float ye, uint32_t color) {
    fixed_t xf = float2fixed(xs);
    fixed_t yf = float2fixed(ys);
    fixed_t x2f = float2fixed(xe);
    fixed_t y2f = float2fixed(ye);
    int16_t dx = round_fixed(x2f - xf);
    int16_t dy = round_fixed(y2f - yf);
    int16_t x = round_fixed(xf);
    int16_t y = round_fixed(yf);
    int16_t x2 = round_fixed(x2f);
    int16_t y2 = round_fixed(y2f);
    scr->in_progress++;
    if (dx == 0 && dy == 0) {
        scr->draw_pixel(scr, x, y, color);
        if (!--scr->in_progress) scr->update_screen(scr, x, y, x, y);
        return;
    } else if (dx == 0) {
        if (dy < 0) {
            y = y2;
            dy = -dy;
        }
        scr->fast_vline(scr, x, y, dy + 1, color);
        if (!--scr->in_progress) scr->update_screen(scr, x, y, x2, y2);
        return;
    } else if (dy == 0) {
        if (dx < 0) {
            x = x2;
            dx = -dx;
        }
        scr->fast_hline(scr, x, y, dx + 1, color);
        if (!--scr->in_progress) scr->update_screen(scr, x, y, x2, y2);
        return;
    }
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    if (dx == dy) {
        dx = x2 - x > 0 ? 1 : -1;
        dy = y2 - y > 0 ? 1 : -1;
        for (int16_t x1 = x, y1 = y; x1 != x2 + dx; x1 += dx, y1 += dy) {
            scr->draw_pixel(scr, x1, y1, color);
        }
        if (!--scr->in_progress) scr->update_screen(scr, x, y, x2, y2);
        return;
    }
    if (dx > dy) {
        int16_t sx = x2 > x ? 1 : -1;
        fixed_t sy = float2fixed((float) (y2f - yf) / (x2f > xf ? x2f - xf : xf - x2f));
        fixed_t y1fx = yf;
        int16_t x1 = x;
        int16_t y1 = y;
        int16_t sp = x;
        while (1) {
            y1fx += sy;
            int16_t yni = round_fixed(y1fx);
            if (yni != y1 || x1 == x2) {
                int16_t ls = sx < 0 ? x1 : sp;
                int16_t lw = sx < 0 ? sp - x1 + 1 : x1 - sp + 1;
                scr->fast_hline(scr, ls, y1, lw, color);
                sp = x1 + sx;
                y1 = yni;
            }
            if (x1 == x2) break;
            x1 += sx;
        }
    } else {
        int16_t sy = y2 > y ? 1 : -1;
        fixed_t sx = float2fixed((float) (x2f - xf) / (y2f > yf ? y2f - yf : yf - y2f));
        fixed_t x1fx = xf;
        int16_t y1 = y;
        int16_t x1 = x;
        int16_t sp = y;
        while (1) {
            x1fx += sx;
            int16_t xni = round_fixed(x1fx);
            if (xni != x1 || y1 == y2) {
                int16_t ls = sy < 0 ? y1 : sp;
                int16_t lw = sy < 0 ? sp - y1 + 1 : y1 - sp + 1;
                scr->fast_vline(scr, x1, ls, lw, color);
                sp = y1 + sy;
                x1 = xni;
            }
            if (y1 == y2) break;
            y1 += sy;
        }
    }
    if (!--scr->in_progress) scr->update_screen(scr, x, y, x2, y2);
}

static uint32_t dgx_draw_vline_mask(dgx_screen_t *scr, int16_t x, int16_t y, int16_t y2, uint32_t color, uint32_t bg, uint32_t mask,
        uint8_t mask_bits) {
    uint32_t rmask;
    int16_t dy = y2 - y;
    if (y > y2) {
        y = y2;
        dy = -dy;
        uint8_t rb = (dy + 1) % mask_bits;
        rmask = ror_nbits(mask, mask_bits, rb);
        uint32_t revmask = rol_bits(rmask, mask_bits);
        mask = 0;
        for (uint8_t i = 0; i < mask_bits; ++i) {
            mask <<= 1;
            mask |= revmask & 1;
            revmask >>= 1;
        }
    } else {
        uint8_t rb = (dy + 1) % mask_bits;
        rmask = ror_nbits(mask, mask_bits, rb);
    }
    scr->fill_vline(scr, x, y, dy + 1, color, bg, mask, mask_bits);
    return rmask;
}

static uint32_t dgx_draw_hline_mask(dgx_screen_t *scr, int16_t x, int16_t y, int16_t x2, uint32_t color, uint32_t bg, uint32_t mask,
        uint8_t mask_bits) {
    uint32_t rmask;
    int16_t dx = x2 - x;
    if (x > x2) {
        x = x2;
        dx = -dx;
        uint8_t rb = (dx + 1) % mask_bits;
        rmask = ror_nbits(mask, mask_bits, rb);
        uint32_t revmask = rol_bits(rmask, mask_bits);
        mask = 0;
        for (uint8_t i = 0; i < mask_bits; ++i) {
            mask <<= 1;
            mask |= revmask & 1;
            revmask >>= 1;
        }
    } else {
        uint8_t rb = (dx + 1) % mask_bits;
        rmask = ror_nbits(mask, mask_bits, rb);
    }
    scr->fill_hline(scr, x, y, dx + 1, color, bg, mask, mask_bits);
    return rmask;
}

uint32_t dgx_draw_line_mask(dgx_screen_t *scr, int16_t x, int16_t y, int16_t x2, int16_t y2, uint32_t color, uint32_t bg, uint32_t mask,
        uint8_t mask_bits) {
    int16_t dx;
    int16_t dy;
    dx = x2 - x;
    dy = y2 - y;
    int16_t sx = dx > 0 ? 1 : -1;
    int16_t sy = dy > 0 ? 1 : -1;
    const uint32_t test_bit = 1;
    scr->in_progress++;
    uint32_t rmask;
    if (dx == 0 && dy == 0) {
        uint32_t c = mask & test_bit ? color : bg;
        scr->draw_pixel(scr, x, y, c);
        if (!--scr->in_progress) scr->update_screen(scr, x, y, x2, y2);
        return ror_bits(mask, mask_bits);
    } else if (dx == 0) {
        rmask = dgx_draw_vline_mask(scr, x, y, y2, color, bg, mask, mask_bits);
        if (!--scr->in_progress) scr->update_screen(scr, x, y, x2, y2);
        return rmask;
    } else if (dy == 0) {
        rmask = dgx_draw_hline_mask(scr, x, y, x2, color, bg, mask, mask_bits);
        if (!--scr->in_progress) scr->update_screen(scr, x, y, x2, y2);
        return rmask;
    }
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    if (dx == dy) {
        for (int16_t x1 = x, y1 = y; x1 != x2 + sx; x1 += sx, y1 += sy) {
            uint32_t c = mask & test_bit ? color : bg;
            scr->draw_pixel(scr, x1, y1, c);
            mask = ror_bits(mask, mask_bits);
        }
        if (!--scr->in_progress) scr->update_screen(scr, x, y, x2, y2);
        return mask;
    }
    int16_t err;
    if (dx > dy) {
        err = dx / 2;
        for (int16_t y1 = y, x1 = x, sp = x; x1 != x2 + sx; x1 += sx) {
            err += dy;
            if (err > dx || x1 == x2) {
                mask = dgx_draw_hline_mask(scr, sp, y1, x1, color, bg, mask, mask_bits);
                sp = x1 + sx;
                y1 += sy;
                err -= dx;
            }
        }
    } else {
        err = dy / 2;
        for (int16_t y1 = y, x1 = x, sp = y; y1 != y2 + sy; y1 += sy) {
            err += dx;
            if (err > dy || y1 == y2) {
                mask = dgx_draw_vline_mask(scr, x1, sp, y1, color, bg, mask, mask_bits);
                sp = y1 + sy;
                x1 += sx;
                err -= dy;
            }
        }
    }
    if (!--scr->in_progress) scr->update_screen(scr, x, y, x2, y2);
    return mask;
}

void dgx_draw_circle(dgx_screen_t *scr, int16_t x, int16_t y, int16_t r, uint32_t color) {
    if (r < 0) return;
    scr->in_progress++;
    if (r == 0) {
        scr->draw_pixel(scr, x, y, color);
        if (!--scr->in_progress) scr->update_screen(scr, x, y, x, y);
        return;
    }
    int16_t xs = 0;
    int16_t ys = r;
    int16_t error = 0;
    int16_t dx = 1, dy = 2 * r - 1;
    scr->draw_pixel(scr, x, y + r, color);
    scr->draw_pixel(scr, x, y - r, color);
    while (ys > 0) {
        if (dx < dy) {
            ++xs;
            error -= dx;
            dx += 2;
            if (-error > dy / 2) {
                --ys;
                error += dy;
                dy -= 2;
            }
        } else if (dy < dx) {
            --ys;
            error += dy;
            dy -= 2;
            if (error > dx / 2) {
                ++xs;
                error -= dx;
                dx += 2;
            }
        } else {
            ++xs;
            --ys;
            error += dy - dx;
            dx += 2;
            dy -= 2;
        }
        scr->draw_pixel(scr, x + xs, y + ys, color);
        scr->draw_pixel(scr, x - xs, y + ys, color);
        if (ys != 0) {
            scr->draw_pixel(scr, x - xs, y - ys, color);
            scr->draw_pixel(scr, x + xs, y - ys, color);
        }
    }
    if (!--scr->in_progress) scr->update_screen(scr, x - r, y - r, x + r, y + r);
}

uint32_t dgx_draw_circle_mask(dgx_screen_t *scr, int16_t x, int16_t y, int16_t r, uint32_t color, uint32_t bg, uint32_t mask,
        uint8_t mask_bits) {
    if (r < 0) return mask;
    const uint32_t msb = 1;
    scr->in_progress++;
    if (r == 0) {
        uint32_t c = mask & msb ? color : bg;
        scr->draw_pixel(scr, x, y, c);
        if (!--scr->in_progress) scr->update_screen(scr, x, y, x, y);
        return ror_bits(mask, mask_bits);
    }
    int16_t xs = 0;
    int16_t ys = r;
    int16_t error = 0;
    int16_t dx = 1, dy = 2 * r - 1;
    error = 0;
    enum {
        nodir, vert, hor
    } cline = nodir;
    int16_t px = xs;
    int16_t py = ys;
    int16_t ld = 1;
    while (ys > 0) {
        if (dx < dy) {
            ++xs;
            error -= dx;
            dx += 2;
            if (-error > dy / 2) {
                --ys;
                error += dy;
                dy -= 2;
            }
        } else if (dy < dx) {
            --ys;
            error += dy;
            dy -= 2;
            if (error > dx / 2) {
                ++xs;
                error -= dx;
                dx += 2;
            }
        } else {
            ++xs;
            error += dy - dx;
            dx += 2;
            --ys;
            dy -= 2;
        }
        if (px == xs && (ld == 1 || cline == vert)) {
            ++ld;
            cline = vert;
        } else if (py == ys && (ld == 1 || cline == hor)) {
            ++ld;
            cline = hor;
        } else {
            if (ld == 1) {
                uint32_t c = mask & msb ? color : bg;
                scr->draw_pixel(scr, x + px, y + py, c);
                mask = ror_bits(mask, mask_bits);
            } else {
                if (cline == hor) {
                    mask = dgx_draw_hline_mask(scr, x + px, y + py, x + px + ld - 1, color, bg, mask, mask_bits);
                } else if (cline == vert) {
                    mask = dgx_draw_vline_mask(scr, x + px, y + py, y + py - ld + 1, color, bg, mask, mask_bits);
                }
            }
            ld = 1;
            px = xs;
            py = ys;
            cline = nodir;
        }
    }
    dy = 1;
    dx = 2 * r - 1;
    error = 0;
    while (xs > 0) {
        if (dx < dy) {
            --xs;
            error += dx;
            dx -= 2;
            if (error > dy / 2) {
                --ys;
                error -= dy;
                dy += 2;
            }
        } else if (dy < dx) {
            --ys;
            error -= dy;
            dy += 2;
            if (-error > dx / 2) {
                --xs;
                error += dx;
                dx -= 2;
            }
        } else {
            --xs;
            error += dx;
            error -= dy;
            dx -= 2;
            --ys;
            dy += 2;
        }
        if (px == xs && (ld == 1 || cline == vert)) {
            ++ld;
            cline = vert;
        } else if (py == ys && (ld == 1 || cline == hor)) {
            ++ld;
            cline = hor;
        } else {
            if (ld == 1) {
                uint32_t c = mask & msb ? color : bg;
                scr->draw_pixel(scr, x + px, y + py, c);
                mask = ror_bits(mask, mask_bits);
            } else {
                if (cline == hor) {
                    mask = dgx_draw_hline_mask(scr, x + px, y + py, x + px - ld + 1, color, bg, mask, mask_bits);
                } else if (cline == vert) {
                    mask = dgx_draw_vline_mask(scr, x + px, y + py, y + py - ld + 1, color, bg, mask, mask_bits);
                }
            }
            ld = 1;
            px = xs;
            py = ys;
            cline = nodir;
        }
    }
    dx = 1;
    dy = 2 * r - 1;
    error = 0;
    while (ys < 0) {
        if (dx < dy) {
            --xs;
            error -= dx;
            dx += 2;
            if (-error > dy / 2) {
                ++ys;
                error += dy;
                dy -= 2;
            }
        } else if (dy < dx) {
            ++ys;
            error += dy;
            dy -= 2;
            if (error > dx / 2) {
                --xs;
                error -= dx;
                dx += 2;
            }
        } else {
            --xs;
            ++ys;
            error += dy - dx;
            dx += 2;
            dy -= 2;
        }
        if (px == xs && (ld == 1 || cline == vert)) {
            ++ld;
            cline = vert;
        } else if (py == ys && (ld == 1 || cline == hor)) {
            ++ld;
            cline = hor;
        } else {
            if (ld == 1) {
                uint32_t c = mask & msb ? color : bg;
                scr->draw_pixel(scr, x + px, y + py, c);
                mask = ror_bits(mask, mask_bits);
            } else {
                if (cline == hor) {
                    mask = dgx_draw_hline_mask(scr, x + px, y + py, x + px - ld + 1, color, bg, mask, mask_bits);
                } else if (cline == vert) {
                    mask = dgx_draw_vline_mask(scr, x + px, y + py, y + py + ld - 1, color, bg, mask, mask_bits);
                }
            }
            ld = 1;
            px = xs;
            py = ys;
            cline = nodir;
        }
    }
    dy = 1;
    dx = 2 * r - 1;
    error = 0;
    while (xs < 0) {
        if (dx < dy) {
            ++xs;
            error += dx;
            dx -= 2;
            if (error > 0 && error > dy / 2) {
                ++ys;
                error -= dy;
                dy += 2;
            }
        } else if (dy < dx) {
            ++ys;
            error -= dy;
            dy += 2;
            if (error < 0 && -error > dx / 2) {
                ++xs;
                error += dx;
                dx -= 2;
            }
        } else {
            ++xs;
            error += dx - dy;
            dx -= 2;
            ++ys;
            dy += 2;
        }
        if (xs < 0 && px == xs && (ld == 1 || cline == vert)) {
            ++ld;
            cline = vert;
        } else if (xs < 0 && py == ys && (ld == 1 || cline == hor)) {
            ++ld;
            cline = hor;
        } else {
            if (ld == 1) {
                uint32_t c = mask & msb ? color : bg;
                scr->draw_pixel(scr, x + px, y + py, c);
                mask = ror_bits(mask, mask_bits);
            } else {
                if (cline == hor) {
                    mask = dgx_draw_hline_mask(scr, x + px, y + py, x + px + ld - 1, color, bg, mask, mask_bits);
                } else if (cline == vert) {
                    mask = dgx_draw_vline_mask(scr, x + px, y + py, y + py + ld - 1, color, bg, mask, mask_bits);
                }
            }
            ld = 1;
            px = xs;
            py = ys;
            cline = nodir;
        }
    }
    if (!--scr->in_progress) scr->update_screen(scr, x - r, y - r, x + r, y + r);
    return mask;
}

void dgx_fill_rectangle(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color) {
    scr->fill_rectangle(scr, x, y, w, h, color);
}

void dgx_draw_circle_solid(dgx_screen_t *scr, int16_t x, int16_t y, int16_t r, uint32_t color) {
    if (r < 0) return;
    scr->in_progress++;
    if (r == 0) {
        scr->draw_pixel(scr, x, y, color);
        if (!--scr->in_progress) scr->update_screen(scr, x, y, x, y);
        return;
    }
    int16_t xs = 0;
    int16_t ys = r;
    int16_t px = xs;
    int16_t error = 0;
    int16_t dx = 1, dy = 2 * r - 1;
    scr->fast_vline(scr, x, y - r, 2 * r + 1, color);
    while (ys > 0) {
        if (dx < dy) {
            ++xs;
            error -= dx;
            dx += 2;
            if (-error > dy / 2) {
                --ys;
                error += dy;
                dy -= 2;
            }
        } else if (dy < dx) {
            --ys;
            error += dy;
            dy -= 2;
            if (error > dx / 2) {
                ++xs;
                error -= dx;
                dx += 2;
            }
        } else {
            ++xs;
            --ys;
            error += dy - dx;
            dx += 2;
            dy -= 2;
        }
        if (px != xs) {
            scr->fast_vline(scr, x + xs, y - ys, 2 * ys + 1, color);
            scr->fast_vline(scr, x - xs, y - ys, 2 * ys + 1, color);
            px = xs;
        }
    }
    if (!--scr->in_progress) scr->update_screen(scr, x - r, y - r, x + r, y + r);
}

#define SORT3PUD(x0, y0, x1, y1, x2, y2) \
    do { \
        if(y0 > y1) { \
            SWAP(y0, y1);\
            SWAP(x0, x1);\
        } \
        if(y1 > y2) { \
            SWAP(y2, y1);\
            SWAP(x2, x1);\
        } \
        if(y0 > y1) { \
            SWAP(y0, y1);\
            SWAP(x0, x1);\
        } \
    } while(0)

void dgx_draw_triangle_solid(dgx_screen_t *scr, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint32_t color) {
    int16_t a;
    int16_t b;
    int16_t y;
    int16_t last;

    // Sort coordinates by Y order (y2 >= y1 >= y0)
    SORT3PUD(x0, y0, x1, y1, x2, y2);
    scr->in_progress++;
    if (y0 == y2) { // Handle awkward all-on-same-line case as its own thing
        a = b = x0;
        if (x1 < a)
            a = x1;
        else if (x1 > b) b = x1;
        if (x2 < a)
            a = x2;
        else if (x2 > b) b = x2;
        scr->fast_hline(scr, a, y0, b - a + 1, color);
        if (!--scr->in_progress) scr->update_screen(scr, a, y0, b, y0);
        return;
    }

    int16_t dx01 = x1 - x0;
    int16_t dy01 = y1 - y0;
    int16_t dx02 = x2 - x0;
    int16_t dy02 = y2 - y0;
    int16_t dx12 = x2 - x1;
    int16_t dy12 = y2 - y1;
    int32_t sa = 0;
    int32_t sb = 0;

    // For upper part of triangle, find scanline crossings for segments
    // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
    // is included here (and second loop will be skipped, avoiding a /0
    // error there), otherwise scanline y1 is skipped here and handled
    // in the second loop...which also avoids a /0 error here if y0=y1
    // (flat-topped triangle).
    if (y1 == y2)
        last = y1;  // Include y1 scanline
    else
        last = y1 - 1; // Skip it

    for (y = y0; y <= last; y++) {
        a = x0 + sa / dy01;
        b = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;

        if (a > b) SWAP(a, b);
        scr->fast_hline(scr, a, y, b - a + 1, color);
    }

    // For lower part of triangle, find scanline crossings for segments
    // 0-2 and 1-2.  This loop is skipped if y1=y2.
    sa = dx12 * (y - y1);
    sb = dx02 * (y - y0);
    for (; y <= y2; y++) {
        a = x1 + sa / dy12;
        b = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;

        if (a > b) SWAP(a, b);
        scr->fast_hline(scr, a, y, b - a + 1, color);
    }
    a = b = x0;
    if (x1 < a)
        a = x1;
    else if (x1 > b) b = x1;
    if (x2 < a)
        a = x2;
    else if (x2 > b) b = x2;
    if (!--scr->in_progress) scr->update_screen(scr, a, y0, b, y2);
}

void dgx_draw_line_thick(dgx_screen_t *scr, int16_t x, int16_t y, int16_t x2, int16_t y2, int16_t width, uint32_t color) {
    int16_t dx;
    int16_t dy;
    if (width < 1) return;
    dx = x2 - x;
    dy = y2 - y;
    int16_t pw = width / 2;
    int16_t mw = width - pw - 1;
    scr->in_progress++;
    if (dx == 0 && dy == 0) {
        dgx_draw_circle_solid(scr, x, y, (mw ? mw : pw), color);
        if (!--scr->in_progress) scr->update_screen(scr, x - pw, y - pw, x + pw, y + pw);
        return;
    } else if (dx == 0) {
        if (dy < 0) {
            y = y2;
            dy = -dy;
            SWAP(mw, pw);
        }
        scr->fill_rectangle(scr, x - mw, y, width, dy + 1, color);
        if (!--scr->in_progress) scr->update_screen(scr, x - mw, y, x + pw, y2);
        return;
    } else if (dy == 0) {
        if (dx < 0) {
            x = x2;
            dx = -dx;
            SWAP(mw, pw);
        }
        scr->fill_rectangle(scr, x, y - mw, dx + 1, width, color);
        if (!--scr->in_progress) scr->update_screen(scr, x, y - mw, x2, y2 + pw);
        return;
    }
    float s = sqrtf((float) ((int32_t) dx * dx) + ((int32_t) dy * dy));
    int16_t mx = lroundf(mw * (int32_t) dy / s);
    int16_t px = lroundf(pw * (int32_t) -dy / s);
    int16_t my = lroundf(mw * (int32_t) -dx / s);
    int16_t py = lroundf(pw * (int32_t) dx / s);
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    int16_t err;
    if (dx >= dy) {
        if (x > x2) {
            SWAP(x, x2);
            SWAP(y, y2);
            SWAP(mx, px);
            SWAP(my, py);
        }
        int16_t ltx0 = x + mx;
        int16_t lty0 = y + my;
        int16_t ltx1 = x + px;
        int16_t lty1 = y + py;
        int16_t rtx0 = x2 + mx;
        int16_t rty0 = y2 + my;
        int16_t rtx1 = x2 + px;
        int16_t rty1 = y2 + py;

        int16_t lty2;
        int16_t ltx2;
        int16_t rtx2;
        int16_t rty2;
        int16_t usy;
        int16_t dsy;
        if (ltx1 > ltx0) { // backslash
            ltx2 = ltx1;
            rtx2 = rtx0;
            lty2 = lty0 - (dy * ((int32_t) ltx1 - ltx0)) / dx;
            rty2 = rty1 + (dy * ((int32_t) rtx1 - rtx0)) / dx;
            int16_t yt = y - (dy * (int32_t) (ltx1 - x)) / dx;
            usy = yt - lty2;
            dsy = lty1 - lty2 + 1;
        } else if (ltx0 > ltx1) { // slash
            ltx2 = ltx0;
            lty2 = lty1 + (dy * ((int32_t) ltx0 - ltx1)) / dx;
            rtx2 = rtx1;
            rty2 = rty0 - (dy * ((int32_t) rtx0 - rtx1)) / dx;
            int16_t yt = y + (dy * (int32_t) (ltx0 - x)) / dx;
            usy = yt - lty0;
            dsy = lty2 - lty0 + 1;
        } else {
            ltx2 = ltx0;
            rtx2 = rtx0;
            lty2 = lty0;
            rty2 = rty0;
            usy = y - lty0;
            dsy = lty1 - lty0 + 1;
        }

        if (ltx1 >= rtx0 || ltx0 >= rtx1) {
            dgx_draw_triangle_solid(scr, ltx0, lty0, ltx1, lty1, rtx1, rty1, color);
            dgx_draw_triangle_solid(scr, rtx0, rty0, rtx1, rty1, ltx0, lty0, color);
            if (y > y2) SWAP(y, y2);
            if (px < 0) px = -px;
            if (py < 0) py = -py;
            if (!--scr->in_progress) scr->update_screen(scr, x - px, y - py, x2 + px, y2 + py);
            return;
        }
        dgx_draw_triangle_solid(scr, ltx0, lty0, ltx1, lty1, ltx2, lty2, color);
        dgx_draw_triangle_solid(scr, rtx0, rty0, rtx1, rty1, rtx2, rty2, color);
        int16_t lx = MAX(ltx1, ltx0) + 1;
        int16_t rx = MIN(rtx1, rtx0) - 1;
        int16_t x1 = x;
        int16_t y1 = y;
        int16_t sp = x;
        int16_t sy = y2 > y ? 1 : -1;
        err = dx / 2;
        do {
            err += dy;
            if (err >= dx || x1 == x2) {
                /*
                 * clip X to MAX(ltx1,ltx0) + 1 .. MIN(rtx0, rtx1)  - 1
                 */
                int16_t lx0 = MAX(lx, sp);
                int16_t rx0 = MIN(rx, x1);
                if (rx0 >= lx0) {
                    scr->fill_rectangle(scr, lx0, y1 - usy, rx0 - lx0 + 1, dsy, color);
                }
                sp = x1 + 1;
                y1 += sy;
                err -= dx;
            }
        } while (x1++ != x2);
    } else {
        if (y > y2) {
            SWAP(x, x2);
            SWAP(y, y2);
            SWAP(px, mx);
            SWAP(py, my);
        }
        /* */
        int16_t ltx0 = x + mx;
        int16_t lty0 = y + my;
        int16_t ltx1 = x + px;
        int16_t lty1 = y + py;
        int16_t rtx0 = x2 + mx;
        int16_t rty0 = y2 + my;
        int16_t rtx1 = x2 + px;
        int16_t rty1 = y2 + py;

        int16_t lty2;
        int16_t ltx2;
        int16_t rtx2;
        int16_t rty2;
        int16_t ls;
        int16_t ds;
        if (lty1 > lty0) { // backslash
            lty2 = lty1;
            rty2 = rty0;
            ltx2 = ltx0 + (dx * ((int32_t) lty1 - lty0)) / dy;
            rtx2 = rtx1 - (dx * ((int32_t) rty1 - rty0)) / dy;
            int16_t xt = x + (dx * (int32_t) (lty1 - y)) / dy;
            ls = xt - ltx1;
            ds = ltx2 - ltx1 + 1;
        } else if (lty0 > lty1) { // slash
            lty2 = lty0;
            rty2 = rty1;
            ltx2 = ltx1 - (dx * ((int32_t) lty0 - lty1)) / dy;
            rtx2 = rtx0 + (dx * ((int32_t) rty0 - rty1)) / dy;
            int16_t xt = x - (dx * (int32_t) (y - lty1)) / dy;
            ls = xt - ltx2;
            ds = ltx0 - ltx2 + 1;
        } else {
            ltx2 = ltx0;
            rtx2 = rtx0;
            lty2 = lty0;
            rty2 = rty0;
            ls = x - ltx1;
            ds = ltx0 - ltx1 + 1;
        }
        if (lty1 >= rty0 || lty0 >= rty1) {
            dgx_draw_triangle_solid(scr, ltx0, lty0, ltx1, lty1, rtx1, rty1, color);
            dgx_draw_triangle_solid(scr, rtx0, rty0, rtx1, rty1, ltx0, lty0, color);
            if (x > x2) SWAP(x, x2);
            if (px < 0) px = -px;
            if (py < 0) py = -py;
            if (!--scr->in_progress) scr->update_screen(scr, x - px, y - py, x2 + px, y2 + py);
            return;
        }
        dgx_draw_triangle_solid(scr, ltx0, lty0, ltx1, lty1, ltx2, lty2, color);
        dgx_draw_triangle_solid(scr, rtx0, rty0, rtx1, rty1, rtx2, rty2, color);
        int16_t ly = MAX(lty1, lty0) + 1;
        int16_t my = MIN(rty1, rty0) - 1;

        /* */
        int16_t sx = x2 > x ? 1 : -1;
        int16_t x1 = x;
        int16_t y1 = y;
        int16_t sp = y;
        err = dy / 2;
        do {
            err += dx;
            if (err >= dy || y1 == y2) {
                int16_t ly0 = MAX(ly, sp);
                int16_t ry0 = MIN(my, y1);
                if (ry0 >= ly0) {
                    scr->fill_rectangle(scr, x1 - ls, ly0, ds, ry0 - ly0 + 1, color);
                }
                sp = y1 + 1;
                x1 += sx;
                err -= dy;
            }
        } while (y1++ != y2);
    }
    if (!--scr->in_progress) scr->update_screen(scr, x, y, x2, y2);
}

