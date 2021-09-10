#include "dgx_screen.h"
uint8_t* dgx_fill_buf_value(uint8_t *lp, int16_t idx, uint32_t value, uint8_t lenbits) {
    if (lenbits == 1) {
        return dgx_fill_buf_value_1(lp, idx, value);
    } else if (lenbits <= 8) {
        *lp++ = value & 255;
    } else if (lenbits <= 12) {
        return dgx_fill_buf_value_12(lp, idx, value);
    } else if (lenbits <= 16) {
        *lp++ = (value & 0xff00) >> 8;
        *lp++ = value & 255;
    } else if (lenbits <= 24) {
        *lp++ = (value & 0xff0000) >> 16;
        *lp++ = (value & 0xff00) >> 8;
        *lp++ = value & 255;
    } else {
        *lp++ = (value & 0xff000000) >> 24;
        *lp++ = (value & 0xff0000) >> 16;
        *lp++ = (value & 0xff00) >> 8;
        *lp++ = value & 255;
    }
    return lp;
}

uint32_t dgx_read_buf_value(uint8_t **lp, int16_t idx, uint8_t lenbits) {
    if (lenbits == 1) {
        return dgx_read_buf_value_1(lp, idx);
    } else if (lenbits <= 8) {
        return dgx_read_buf_value_8(lp, idx);
    } else if (lenbits <= 12) {
        return dgx_read_buf_value_12(lp, idx);
    } else if (lenbits <= 16) {
        return dgx_read_buf_value_16(lp, idx);
    } else if (lenbits <= 24) {
        return dgx_read_buf_value_24(lp, idx);
    } else {
        return dgx_read_buf_value_32(lp, idx);
    }
}

