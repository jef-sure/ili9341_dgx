#ifndef _DGX_FONT_H_
#define _DGX_FONT_H_

#include <stdint.h>
#include <stddef.h>
#include "dgx_screen.h"
#include "dgx_bitmap.h"

#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

typedef struct {
    const uint8_t *bitmap; ///< Pointer into GFXfont->bitmap
    int16_t width;///< Bitmap dimensions in pixels
    int16_t height;///< Bitmap dimensions in pixels
    int16_t xAdvance;///< Distance to advance cursor (x axis)
    int16_t xOffset;
    int16_t yOffset;
}glyph_t;

typedef struct {
    int first, number;
    const glyph_t *glyphs;
}glyph_array_t;

typedef struct {
    const glyph_array_t *glyph_ranges;
    int16_t yAdvance, yOffsetLowest, xWidest, xWidthAverage;
}dgx_font_t;

uint32_t decodeUTF8next(const char *chr, size_t *idx);
const glyph_t* dgx_font_find_glyph(uint32_t codePoint, dgx_font_t *font, int16_t *xAdvance);
int16_t dgx_font_char_to_screen(dgx_screen_t *scr, int16_t x, int16_t y, uint32_t codePoint, uint32_t color, uint32_t bg_color, dgx_orientation_t xdir, dgx_orientation_t ydir, bool swap_xy, dgx_font_t *font);
int16_t dgx_font_char_to_bw_bitmap(dgx_bw_bitmap_t *bmap, int16_t x, int16_t y, uint32_t codePoint, dgx_font_t *font);
int16_t dgx_font_string_bounds(const char *str, dgx_font_t *font, int16_t *ycorner, int16_t *height);
void dgx_font_string_utf8_screen(dgx_screen_t *scr, int16_t x, int16_t y, const char *str, uint32_t color, uint32_t bg_color, dgx_orientation_t xdir, dgx_orientation_t ydir, bool swap_xy, dgx_font_t *font);
void dgx_draw_string_utf8_bitmap(dgx_bw_bitmap_t *bmap, int16_t x, int16_t y, const char *str, dgx_font_t *font);

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on
#endif

#endif /* _DGX_FONT_H_ */
