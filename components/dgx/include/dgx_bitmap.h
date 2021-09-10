#ifndef _DGX_BITMAP_H_
#define _DGX_BITMAP_H_

#include <stdint.h>
#include <stddef.h>

#include "dgx_screen.h"
#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif
typedef struct {
    uint8_t *bitmap;
    int16_t width;
    int16_t height;
}dgx_bw_bitmap_t;

dgx_bw_bitmap_t dgx_bw_bitmap_allocate(int16_t width, int16_t height);
void dgx_bw_bitmap_free(dgx_bw_bitmap_t *bmap);
void dgx_bw_bitmap_set8_bits_(dgx_bw_bitmap_t *bmap_dst, int16_t x, int16_t y, uint8_t val);
void dgx_bw_bitmap_set_pixel(dgx_bw_bitmap_t *bmap, int16_t x, int16_t y, uint8_t value);
bool dgx_bw_bitmap_get_pixel(dgx_bw_bitmap_t *bmap, int16_t x, int16_t y);
void dgx_bw_bitmap_to_screen(dgx_screen_t *scr, dgx_bw_bitmap_t *bmap, int16_t x, int16_t y, uint32_t color, uint32_t bg_color, dgx_orientation_t xdir, dgx_orientation_t ydir, bool swap_xy);
void dgx_bw_bitmap_to_screen_transp(dgx_screen_t *scr, dgx_bw_bitmap_t *bmap, int16_t x, int16_t y, uint32_t color, dgx_orientation_t xdir, dgx_orientation_t ydir, bool swap_xy);
void dgx_bw_bitmap_to_bitmap(dgx_bw_bitmap_t *bmap_dst, int16_t x, int16_t y, dgx_bw_bitmap_t *bmap_src);
void dgx_bw_bitmap_clear(dgx_bw_bitmap_t *bmap, bool invert);
#ifdef __cplusplus
// @formatter:off
}
// @formatter:on
#endif
#endif /* _DGX_BITMAP_H_ */
