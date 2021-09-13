/*
 * MIT License
 *
 * Copyright (c) 2021 Anton Petrusevich
 *
 */

#ifndef _DGX_INTERSCREEN_H_
#define _DGX_INTERSCREEN_H_

#include "dgx_screen.h"
#include "dgx_v_screen.h"

#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

typedef enum {
    BitbltSet, BitbltOr, BitbltAnd, BitbltXor, BitbltAlpha
}dgx_bitblt_rop_t;

void dgx_vscreen_to_screen(dgx_screen_t *scr_dst, int16_t x_dst, int16_t y_dst, dgx_screen_t *scr_src);
void dgx_vscreen_to_vscreen(dgx_screen_t *scr_dst, int16_t x_dst, int16_t y_dst, dgx_screen_t *scr_src, dgx_bitblt_rop_t rop);
void dgx_copy_region_from_vscreen(dgx_screen_t *scr_dst, int16_t x_dst, int16_t y_dst, dgx_screen_t *scr_src, int16_t x_src, int16_t y_src, int16_t width, int16_t height, uint32_t outer_color);
dgx_screen_t* dgx_new_vscreen_from_region(dgx_screen_t *scr_src, int16_t x, int16_t y, int16_t width, int16_t height);
void dgx_vscreen8_to_screen16(dgx_screen_t *scr_dst, int16_t x_dst, int16_t y_dst, dgx_screen_t *scr_src, uint16_t *lut);
dgx_screen_t* dgx_clone_vscreen_clear(dgx_screen_t *scr_src);
bool dgx_copy_vscreen(dgx_screen_t *scr_dst, dgx_screen_t *scr_src);

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on
#endif
#endif /* _DGX_INTERSCREEN_H_ */
