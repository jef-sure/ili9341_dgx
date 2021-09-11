/*
 * MIT License
 *
 * Copyright (c) 2021 Anton Petrusevich
 *
 */

#ifndef __TTF_SCREEN_H__
#define __TTF_SCREEN_H__
#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/i2c.h"

struct dgx_screen_;
typedef void (*dgx_set_area_func)(struct dgx_screen_ *scr, uint16_t caset_lo, uint16_t caset_hi, uint16_t raset_lo, uint16_t raset_hi);
typedef void (*dgx_write_area_func)(struct dgx_screen_ *scr, uint8_t *data, uint32_t lenbits);
typedef void (*dgx_write_value_func)(struct dgx_screen_ *scr, uint32_t value);
typedef void (*dgx_wait_screen_buffer_func)(struct dgx_screen_ *scr);
typedef void (*dgx_draw_pixel_func)(struct dgx_screen_ *scr, int16_t x, int16_t y, uint32_t color);
typedef uint32_t (*dgx_read_pixel_func)(struct dgx_screen_ *scr, int16_t x, int16_t y);
typedef void (*dgx_fill_line_func)(struct dgx_screen_ *scr, int16_t x, int16_t y, int16_t w, uint32_t color, uint32_t bg, uint32_t mask, uint8_t mask_bits);
typedef void (*dgx_fill_rectangle_func)(struct dgx_screen_ *scr, int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color);
typedef void (*dgx_update_area_func)(struct dgx_screen_ *scr, int16_t x1, int16_t y1, int16_t x2, int16_t y2);
typedef void (*dgx_fast_vline_func)(struct dgx_screen_ *scr, int16_t x, int16_t y, int16_t h, uint32_t color);
typedef void (*dgx_fast_hline_func)(struct dgx_screen_ *scr, int16_t x, int16_t y, int16_t w, uint32_t color);
typedef void (*dgx_screen_destroy_func)(struct dgx_screen_ *scr);

typedef enum {
    BottomTop = -1, TopBottom = 1, LeftRight = 1, RightLeft = -1
}dgx_orientation_t;

typedef enum {
    RGB, BGR
}dgx_color_order_t;

typedef enum {
    BusSPI, BusI2C, BusP8
}dgx_bus_t;

typedef struct dgx_screen_ {
    int16_t width;
    int16_t height;
    uint8_t color_bits;
    uint8_t lcd_ram_bits;
    dgx_orientation_t dir_x;
    dgx_orientation_t dir_y;
    dgx_color_order_t rgb_order;
    bool swap_xy;
    gpio_num_t dc;
    gpio_num_t cs;
    uint8_t cmd_col_set;
    uint8_t cmd_row_set;
    uint8_t cmd_write_data;
    dgx_bus_t bus_type;
    uint16_t cg_row_shift;
    uint16_t cg_col_shift;
    union {
        struct {
            spi_device_handle_t spi;
            spi_transaction_t *trans;
            spi_transaction_t *trans_data;
            int16_t pending_transactions;
            int16_t max_transactions;
            uint32_t max_transfer_sz;
            spi_host_device_t host_id;
            spi_device_interface_config_t devcfg;
        };
        struct {
            i2c_port_t i2c_num;
            uint8_t i2c_address;
        };
        struct {
            gpio_num_t lcd_d0;
            gpio_num_t lcd_d1;
            gpio_num_t lcd_d2;
            gpio_num_t lcd_d3;
            gpio_num_t lcd_d4;
            gpio_num_t lcd_d5;
            gpio_num_t lcd_d6;
            gpio_num_t lcd_d7;
            gpio_num_t lcd_wr;
            gpio_num_t lcd_rd;
        };
    };
    /* */
    bool is_data_free;
    uint8_t *draw_buffer;
    /* Draw buffer length must be at least max(rows, cols) + 32 */
    uint16_t draw_buffer_len;
    uint8_t *frame_buffer;
    uint16_t fb_col_lo;
    uint16_t fb_col_hi;
    uint16_t fb_row_lo;
    uint16_t fb_row_hi;
    uint16_t fb_pointer_x;
    uint16_t fb_pointer_y;
    uint32_t in_progress;
    dgx_set_area_func set_area;
    dgx_write_area_func write_area;
    dgx_write_value_func write_value;
    dgx_wait_screen_buffer_func wait_screen_buffer;
    dgx_draw_pixel_func draw_pixel;
    dgx_read_pixel_func read_pixel;
    dgx_fill_line_func fill_vline;
    dgx_fill_line_func fill_hline;
    dgx_fast_vline_func fast_vline;
    dgx_fast_hline_func fast_hline;
    dgx_fill_rectangle_func fill_rectangle;
    dgx_update_area_func update_screen;
    dgx_screen_destroy_func destroy;
}dgx_screen_t;

static inline uint32_t rol_bits(uint32_t a, uint8_t bits) {
    return (a >> (bits - 1)) | (a << 1);
}

static inline uint32_t ror_bits(uint32_t a, uint8_t bits) {
    return (a << (bits - 1)) | (a >> 1);
}

static inline uint32_t rol_nbits(uint32_t a, uint8_t bits, uint8_t n) {
    return (a >> (bits - n)) | (a << n);
}

static inline uint32_t ror_nbits(uint32_t a, uint8_t bits, uint8_t n) {
    return (a << (bits - n)) | (a >> n);
}

#define DGX_RGB_12(r,g,b) ((((r) & 0xf0u) << 8) | (((g) & 0xf0u) << 4) | ((b) & 0xf0u))
#define DGX_RGB_16(r,g,b) ((((r) & 0xf8u) << 8) | (((g) & 0xfcu) << 3) | (((b) & 0xf8u) >> 3))
#define DGX_RGB_18(r,g,b) ((((r) & 0xfcu) << 16) | (((g) & 0xfcu) << 8) | ((b) & 0xfcu))
#define DGX_RGB_24(r,g,b) (((r) << 16) | ((g) << 8) | (b))

static inline uint16_t dgx_rgb_to_12(uint8_t r, uint8_t g, uint8_t b) {
    return DGX_RGB_12(r,g,b);
}

static inline uint16_t dgx_rgb_to_16(uint8_t r, uint8_t g, uint8_t b) {
    return DGX_RGB_16(r,g,b);
}

static inline uint32_t dgx_rgb_to_18(uint8_t r, uint8_t g, uint8_t b) {
    return DGX_RGB_18(r,g,b);
}

static inline uint32_t dgx_rgb_to_24(uint8_t r, uint8_t g, uint8_t b) {
    return DGX_RGB_24(r,g,b);
}

static inline uint8_t* dgx_fill_buf_value_1(uint8_t *lp, int16_t idx, uint32_t value) {
    uint8_t mask = 0x80 >> (idx & 7);
    if (value) {
        *lp |= mask;
    } else {
        *lp &= ~mask;
    }
    lp += mask & 1;
    return lp;
}

static inline uint8_t* dgx_fill_buf_value_8(uint8_t *lp, int16_t idx, uint32_t value) {
    *lp = (uint8_t)value;
    return lp + 1;
}

static inline uint8_t* dgx_fill_buf_value_12(uint8_t *lp, int16_t idx, uint32_t value) {
    if ((idx & 1) == 0) {
        *lp++ = (uint8_t)(value >> 8);
        *lp = (value & 0xf0u);
    } else {
        uint8_t pb = *lp & 0xf0u;
        *lp++ = pb | ((value & 0xf000u) >> 12);
        *lp++ = (uint8_t)(value >> 4);;
    }
    return lp;
}

static inline uint8_t* dgx_fill_buf_value_16(uint8_t *lp, int16_t idx, uint32_t value) {
    *lp++ = (uint8_t)(value >> 8);
    *lp++ = (uint8_t)value;
    return lp;
}

static inline uint8_t* dgx_fill_buf_value_24(uint8_t *lp, int16_t idx, uint32_t value) {
    *lp++ = (uint8_t)(value >> 16);
    *lp++ = (uint8_t)(value >> 8);
    *lp++ = (uint8_t)value;
    return lp;
}

static inline uint8_t* dgx_fill_buf_value_32(uint8_t *lp, int16_t idx, uint32_t value) {
    *lp++ = (uint8_t)(value >> 24);
    *lp++ = (uint8_t)(value >> 16);
    *lp++ = (uint8_t)(value >> 8);
    *lp++ = (uint8_t)value;
    return lp;
}

static inline uint32_t dgx_read_buf_value_1(uint8_t **lp, int16_t idx) {
    uint8_t mask = 0x80 >> (idx & 7);
    uint32_t value = !!(**lp & mask);
    *lp += mask & 1;
    return value;
}

static inline uint32_t dgx_read_buf_value_8(uint8_t **lp, int16_t idx) {
    uint32_t value = **lp;
    ++*lp;
    return value;
}

static inline uint32_t dgx_read_buf_value_12(uint8_t **lp, int16_t idx) {
    uint32_t value = **lp << 8;
    ++*lp;
    if ((idx & 1) == 0) {
        value |= **lp & 0xf0;
    } else {
        value |= **lp;
        value <<= 4;
        value &= 0xfff0;
        ++*lp;
    }
    return value;
}

static inline uint32_t dgx_read_buf_value_16(uint8_t **lp, int16_t idx) {
    uint32_t value = (**lp << 8) | (*(*lp+1));
    *lp += 2;
    return value;
}

static inline uint32_t dgx_read_buf_value_24(uint8_t **lp, int16_t idx) {
    uint32_t value = (**lp << 16) | (*(*lp+1) << 8) | (*(*lp+2));
    *lp += 3;
    return value;
}

static inline uint32_t dgx_read_buf_value_32(uint8_t **lp, int16_t idx) {
    uint32_t value = (**lp << 24) | (*(*lp+1) << 16) | (*(*lp+2)<<8) | (*(*lp+3));
    *lp += 4;
    return value;
}

uint8_t* dgx_fill_buf_value(uint8_t *lp, int16_t idx, uint32_t value, uint8_t lenbits);
uint32_t dgx_read_buf_value(uint8_t **lp, int16_t idx, uint8_t lenbits);
void dgx_fill_hline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, uint32_t color, uint32_t bg, uint32_t mask, uint8_t mask_bits);
void dgx_fill_vline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t h, uint32_t color, uint32_t bg, uint32_t mask, uint8_t mask_bits);
void dgx_fast_vline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t h, uint32_t color);
void dgx_fast_hline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, uint32_t color);

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on

#endif

#endif /* __TTF_SCREEN_H__ */
