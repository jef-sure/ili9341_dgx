/*
 * MIT License
 *
 * Copyright (c) 2021 Anton Petrusevich
 *
 */

#include <string.h>

#include "dgx_spi_screen.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"

static inline void* set_dc_pin(gpio_num_t dc, uint8_t val) {
    return (void*) ((val & 1u) | ((uint32_t) dc << 1));
}

static inline gpio_num_t get_dc_pin(void *user_ptr) {
    uint32_t dc = (uint32_t) user_ptr;
    return dc;
}

void dgx_spi_sync_send_cmd(dgx_screen_t *scr, const uint8_t cmd) {
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length = 8;                     //Command is 8 bits
    t.flags = SPI_TRANS_USE_TXDATA;
    t.tx_data[0] = cmd;               //The data is the cmd itself
    t.user = set_dc_pin(scr->dc, 0);                //D/C needs to be set to 0
    ret = spi_device_transmit(scr->spi, &t);  //Transmit!
    assert(ret == ESP_OK);            //Should have had no issues.
}

void dgx_spi_sync_send_commans(dgx_screen_t *scr, const uint8_t *cmds, int len) {
    esp_err_t ret;
    spi_transaction_t t;
    if (len == 0) return;             //no need to send anything
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length = len * 8;    //Len is in bytes, transaction length is in bits.
    t.tx_buffer = cmds;               //Data
    t.user = set_dc_pin(scr->dc, 0);                //D/C needs to be set to 0
    ret = spi_device_transmit(scr->spi, &t);  //Transmit!
    assert(ret == ESP_OK);            //Should have had no issues.
}

//Send data to the LCD. Uses spi_device_transmit, which waits until the transfer is complete.
void dgx_spi_sync_send_data(dgx_screen_t *scr, const uint8_t *data, int len) {
    esp_err_t ret;
    spi_transaction_t t;
    if (len == 0) return;             //no need to send anything
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length = len * 8;    //Len is in bytes, transaction length is in bits.
    t.tx_buffer = data;               //Data
    t.user = set_dc_pin(scr->dc, 1); //D/C needs to be set to 1
    ret = spi_device_transmit(scr->spi, &t);  //Transmit!
    assert(ret == ESP_OK);            //Should have had no issues.
}

//This function is called (in irq context!) just before a transmission starts. It will
//set the D/C line to the value indicated in the user field.
static void dgx_spi_set_dc(spi_transaction_t *t) {
    uint32_t dc = get_dc_pin(t->user);
    if (dc & 1) {
        GPIO.out_w1ts = 1u << (dc >> 1);
    } else {
        GPIO.out_w1tc = 1u << (dc >> 1);
    }
}

static void dgx_spi_set_dc_high(spi_transaction_t *t) {
    uint32_t dc = get_dc_pin(t->user);
    if (dc & 1) {
        GPIO.out1_w1ts.val = 1u << ((dc >> 1u) - 32u);
    } else {
        GPIO.out1_w1tc.val = 1u << ((dc >> 1u) - 32u);
    }
}

static void dgx_spi_init_transactions(dgx_screen_t *scr) {
    for (uint16_t i = 0; i < 5; i++) {
        if ((i & 1) == 0) {
            //Even transfers are commands
            scr->trans[i].length = 8;
            scr->trans[i].user = set_dc_pin(scr->dc, 0);                //D/C needs to be set to 0
        } else {
            //Odd transfers are data
            scr->trans[i].length = 8 * 4;
            scr->trans[i].user = set_dc_pin(scr->dc, 1);               //D/C needs to be set to 1
        }
        scr->trans[i].flags = SPI_TRANS_USE_TXDATA;
    }
    scr->trans_data->user = 0;
}

void dgx_spi_set_area(dgx_screen_t *scr, uint16_t caset_lo, uint16_t caset_hi, uint16_t raset_lo, uint16_t raset_hi) {
    dgx_spi_allocate_queue(scr, 6);
    dgx_spi_crset(scr, caset_lo, caset_hi, raset_lo, raset_hi);
    dgx_spi_write_ram(scr);
}

void dgx_spi_update_screen(dgx_screen_t *scr, int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    return;
}

esp_err_t dgx_spi_init(dgx_screen_t *scr, spi_host_device_t host_id, gpio_num_t mosi, gpio_num_t sclk, gpio_num_t cs,
        gpio_num_t dc, int clock_speed_hz) {
    scr->bus_type = BusSPI;
    scr->cg_col_shift = 0;
    scr->cg_row_shift = 0;
    scr->dc = dc;
    scr->cs = cs;
    scr->pending_transactions = 0;
    scr->max_transactions = 7;
    scr->trans = heap_caps_calloc(6, sizeof(spi_transaction_t), MALLOC_CAP_DMA);
    if (!scr->trans) return ESP_FAIL;
    scr->trans_data = scr->trans + 5;
    scr->is_data_free = true;
    dgx_spi_init_transactions(scr);
    scr->in_progress = 0;
    scr->set_area = dgx_spi_set_area;
    scr->write_area = dgx_spi_write_data;
    scr->write_value = dgx_spi_write_value;
    scr->wait_screen_buffer = dgx_spi_wait_data;
    scr->draw_pixel = dgx_spi_draw_pixel;
    scr->read_pixel = dgx_spi_read_pixel;
    scr->fill_hline = dgx_spi_fill_hline;
    scr->fill_vline = dgx_spi_fill_vline;
    scr->fast_hline = dgx_spi_fast_hline;
    scr->fast_vline = dgx_spi_fast_vline;
    scr->fill_rectangle = dgx_spi_fill_rectangle;
    scr->update_screen = dgx_spi_update_screen;
    spi_bus_config_t buscfg;
    memset(&buscfg, 0, sizeof(buscfg));
    buscfg.miso_io_num = -1;
    buscfg.mosi_io_num = mosi;
    buscfg.sclk_io_num = sclk;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = 4092;
    scr->max_transfer_sz = buscfg.max_transfer_sz;
    memset(&scr->devcfg, 0, sizeof(scr->devcfg));
    scr->devcfg.clock_speed_hz = clock_speed_hz;
    scr->devcfg.spics_io_num = cs;
    scr->devcfg.queue_size = scr->max_transactions;
    if (dc > 31) {
        scr->devcfg.pre_cb = dgx_spi_set_dc_high;
    } else {
        scr->devcfg.pre_cb = dgx_spi_set_dc;
    }
    esp_err_t ret;
//Initialize the SPI bus
    ret = spi_bus_initialize(host_id, &buscfg, host_id);
    assert(ret == ESP_OK);
    if (ret != ESP_OK) return ret;
    scr->host_id = host_id;
    gpio_pad_select_gpio(dc);
    gpio_set_direction(dc, GPIO_MODE_OUTPUT);
    return ESP_OK;
}

void dgx_spi_allocate_queue(dgx_screen_t *scr, uint16_t free_q) {
    spi_transaction_t *rtrans;
    while (scr->pending_transactions && scr->max_transactions - scr->pending_transactions < free_q) {
        esp_err_t ret = spi_device_get_trans_result(scr->spi, &rtrans, portMAX_DELAY);
        assert(ret == ESP_OK);
        scr->pending_transactions--;
        if (rtrans == scr->trans_data) scr->is_data_free = true;
    }
}

void dgx_spi_wait_pending(dgx_screen_t *scr) {
    dgx_spi_allocate_queue(scr, scr->max_transactions);
}

void dgx_spi_crset(dgx_screen_t *scr, uint16_t caset_lo, uint16_t caset_hi, uint16_t raset_lo, uint16_t raset_hi) {
    raset_lo += scr->cg_row_shift;
    raset_hi += scr->cg_row_shift;
    raset_lo += scr->cg_col_shift;
    raset_hi += scr->cg_col_shift;
    if (scr->fb_col_hi != caset_hi || scr->fb_col_lo != caset_lo) {
        esp_err_t ret = spi_device_queue_trans(scr->spi, &scr->trans[0], portMAX_DELAY);
        assert(ret == ESP_OK);
        scr->pending_transactions++;
        scr->trans[1].tx_data[0] = caset_lo >> 8;
        scr->trans[1].tx_data[1] = caset_lo & 255;
        scr->trans[1].tx_data[2] = caset_hi >> 8;
        scr->trans[1].tx_data[3] = caset_hi & 255;
        scr->fb_col_lo = caset_lo;
        scr->fb_col_hi = caset_hi;
        ret = spi_device_queue_trans(scr->spi, &scr->trans[1], portMAX_DELAY);
        assert(ret == ESP_OK);
        scr->pending_transactions++;
    }
    if (scr->fb_row_hi != raset_hi || scr->fb_row_lo != raset_lo) {
        esp_err_t ret = spi_device_queue_trans(scr->spi, &scr->trans[2], portMAX_DELAY);
        assert(ret == ESP_OK);
        scr->pending_transactions++;
        scr->trans[3].tx_data[0] = raset_lo >> 8;
        scr->trans[3].tx_data[1] = raset_lo & 255;
        scr->trans[3].tx_data[2] = raset_hi >> 8;
        scr->trans[3].tx_data[3] = raset_hi & 255;
        scr->fb_row_lo = raset_lo;
        scr->fb_row_hi = raset_hi;
        ret = spi_device_queue_trans(scr->spi, &scr->trans[3], portMAX_DELAY);
        assert(ret == ESP_OK);
        scr->pending_transactions++;
    }
}

void dgx_spi_write_ram(dgx_screen_t *scr) {
    esp_err_t ret = spi_device_queue_trans(scr->spi, &scr->trans[4], portMAX_DELAY);
    assert(ret == ESP_OK);
    scr->pending_transactions++;
}

void dgx_spi_write_data(dgx_screen_t *scr, uint8_t *data, uint32_t lenbits) {
    memset(scr->trans_data, 0, sizeof(spi_transaction_t));
    scr->trans_data->flags = 0;
    scr->trans_data->tx_buffer = data;
    scr->trans_data->length = lenbits + (7 & (8 - (lenbits & 7)));
    scr->trans_data->user = set_dc_pin(scr->dc, 1);
    esp_err_t ret = spi_device_queue_trans(scr->spi, scr->trans_data, portMAX_DELAY);
    assert(ret == ESP_OK);
    scr->is_data_free = false;
    scr->pending_transactions++;
}

void dgx_spi_write_value(dgx_screen_t *scr, uint32_t value) {
    memset(scr->trans_data, 0, sizeof(spi_transaction_t));
    scr->trans_data->flags = SPI_TRANS_USE_TXDATA;
    scr->trans_data->length = scr->lcd_ram_bits + (7 & (8 - (scr->lcd_ram_bits & 7)));
    dgx_fill_buf_value(scr->trans_data->tx_data, 0, value, scr->lcd_ram_bits);
    scr->trans_data->user = set_dc_pin(scr->dc, 1);
    esp_err_t ret = spi_device_queue_trans(scr->spi, scr->trans_data, portMAX_DELAY);
    assert(ret == ESP_OK);
    scr->is_data_free = false;
    scr->pending_transactions++;
}

void dgx_spi_wait_data(dgx_screen_t *scr) {
    if (!scr->is_data_free) dgx_spi_wait_pending(scr);
}

void dgx_spi_draw_pixel(dgx_screen_t *scr, int16_t x, int16_t y, uint32_t color) {
    if (x < 0 || y < 0 || x >= scr->width || y >= scr->height) return;
    dgx_spi_wait_pending(scr);
    dgx_spi_crset(scr, x, x, y, y);
    dgx_spi_write_ram(scr);
    dgx_spi_write_value(scr, color);
    dgx_spi_wait_data(scr);
}

uint32_t dgx_spi_read_pixel(dgx_screen_t *scr, int16_t x, int16_t y) {
    return 0;
}

void dgx_spi_fill_line_buffer(dgx_screen_t *scr, int16_t w, uint32_t color, uint32_t bg, uint32_t mask,
        uint8_t mask_bits) {
    const uint32_t test_bit = 1;
    uint8_t *lp = scr->draw_buffer;
    if (scr->color_bits == 16) {
        color &= 0xffff;
        bg &= 0xffff;
        for (; w-- > 0; mask = ror_bits(mask, mask_bits)) {
            uint16_t c = mask & test_bit ? (uint16_t) color : (uint16_t) bg;
            *lp++ = c >> 8;
            *lp++ = c & 255;
        }
    } else if (scr->color_bits == 18) {
        color &= 0xffffff;
        bg &= 0xffffff;
        for (; w-- > 0; mask = ror_bits(mask, mask_bits)) {
            uint32_t c = mask & test_bit ? color : bg;
            *lp++ = c >> 16;
            *lp++ = c >> 8;
            *lp++ = c & 255;
        }
    } else if (scr->color_bits == 12) {
        color &= 0xfff0u;
        bg &= 0xfff0u;
        for (uint16_t i = 0; w-- > 0; ++i, mask = ror_bits(mask, mask_bits)) {
            uint16_t c = mask & test_bit ? (uint16_t) color : (uint16_t) bg;
            lp = dgx_fill_buf_value_12(lp, i, c);
        }
    }
}

static void dgx_spi_send_line(dgx_screen_t *scr, int16_t w, uint32_t color, uint32_t bg, uint32_t mask,
        uint8_t mask_bits) {
    dgx_spi_wait_data(scr);
    dgx_spi_fill_line_buffer(scr, w, color, bg, mask, mask_bits);
    dgx_spi_write_ram(scr);
    dgx_spi_write_data(scr, scr->draw_buffer, scr->lcd_ram_bits * w);
    dgx_spi_wait_data(scr);
}

void dgx_spi_fill_hline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, uint32_t color, uint32_t bg, uint32_t mask,
        uint8_t mask_bits) {
    if (y < 0 || y >= scr->height || x + w <= 0 || x >= scr->width) return;
    if (x < 0) {
        w = x + w;
        x = 0;
    }
    if (x + w > scr->width) {
        w = scr->width - x;
    }
    dgx_spi_allocate_queue(scr, 6);
    dgx_spi_crset(scr, x, x + w - 1, y, y);
    dgx_spi_send_line(scr, w, color, bg, mask, mask_bits);
}

void dgx_spi_fill_vline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t h, uint32_t color, uint32_t bg, uint32_t mask,
        uint8_t mask_bits) {
    if (x < 0 || x >= scr->width || y + h <= 0 || y >= scr->height) return;
    if (y < 0) {
        h = y + h;
        h = 0;
    }
    if (y + h > scr->height) {
        h = scr->width - y;
    }
    dgx_spi_allocate_queue(scr, 6);
    dgx_spi_crset(scr, x, x, y, y + h - 1);
    dgx_spi_send_line(scr, h, color, bg, mask, mask_bits);
}

void dgx_spi_fill_rectangle(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color) {
    if (y < 0 || w <= 0 || x + w <= 0 || x >= scr->width || h <= 0 || y + h <= 0 || y >= scr->height) return;
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
    uint32_t chunk_len = h * w * scr->lcd_ram_bits;
    uint32_t rsize = chunk_len;
    if (chunk_len > scr->draw_buffer_len * 8) {
        chunk_len = scr->draw_buffer_len * 8;
        uint32_t fb = chunk_len / scr->lcd_ram_bits; // pixels in chunk
        fb &= ~7; // align chunk to 8 pixels
        chunk_len = fb * 8;
    } else {
        chunk_len += (7 & (8 - (chunk_len & 7))); // align to byte boundary
    }
    dgx_spi_allocate_queue(scr, 6);
    dgx_spi_crset(scr, x, x + w - 1, y, y + h - 1);
    dgx_spi_write_ram(scr);
    dgx_spi_wait_data(scr);
    uint8_t *lp = scr->draw_buffer;
    uint32_t pic = chunk_len / scr->lcd_ram_bits;
    for (int16_t i = 0; i < pic; ++i)
        lp = dgx_fill_buf_value(lp, i, color, scr->lcd_ram_bits);
    while (rsize >= 8) {
        if (chunk_len > rsize) chunk_len = rsize;
        dgx_spi_write_data(scr, scr->draw_buffer, chunk_len);
        dgx_spi_wait_data(scr);
        rsize -= chunk_len;
    }
}

static void dgx_spi_send_fast_line(dgx_screen_t *scr, int16_t w, uint32_t color) {
    dgx_spi_wait_data(scr);
    uint8_t *lp = scr->draw_buffer;
    for (int16_t i = 0; i < w; ++i)
        lp = dgx_fill_buf_value(lp, i, color, scr->lcd_ram_bits);
    dgx_spi_write_ram(scr);
    dgx_spi_write_data(scr, scr->draw_buffer, scr->lcd_ram_bits * w);
    dgx_spi_wait_data(scr);
}

void dgx_spi_fast_vline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t h, uint32_t color) {
    if (x < 0 || x >= scr->width || y + h <= 0 || y >= scr->height) return;
    if (y < 0) {
        h = y + h;
        h = 0;
    }
    if (y + h > scr->height) {
        h = scr->width - y;
    }
    dgx_spi_allocate_queue(scr, 6);
    dgx_spi_crset(scr, x, x, y, y + h - 1);
    dgx_spi_send_fast_line(scr, h, color);

}

void dgx_spi_fast_hline(dgx_screen_t *scr, int16_t x, int16_t y, int16_t w, uint32_t color) {
    if (y < 0 || y >= scr->height || x + w <= 0 || x >= scr->width) return;
    if (x < 0) {
        w = x + w;
        x = 0;
    }
    if (x + w > scr->width) {
        w = scr->width - x;
    }
    dgx_spi_allocate_queue(scr, 6);
    dgx_spi_crset(scr, x, x + w - 1, y, y);
    dgx_spi_send_fast_line(scr, w, color);
}

