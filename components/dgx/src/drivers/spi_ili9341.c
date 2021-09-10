#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "dgx_spi_screen.h"

#define ILI9341_TFTWIDTH 240  ///< ILI9341 max TFT width
#define ILI9341_TFTHEIGHT 320 ///< ILI9341 max TFT height

#define ILI9341_NOP 0x00     ///< No-op register
#define ILI9341_SWRESET 0x01 ///< Software reset register
#define ILI9341_RDDID 0x04   ///< Read display identification information
#define ILI9341_RDDST 0x09   ///< Read Display Status

#define ILI9341_SLPIN 0x10  ///< Enter Sleep Mode
#define ILI9341_SLPOUT 0x11 ///< Sleep Out
#define ILI9341_PTLON 0x12  ///< Partial Mode ON
#define ILI9341_NORON 0x13  ///< Normal Display Mode ON

#define ILI9341_RDMODE 0x0A     ///< Read Display Power Mode
#define ILI9341_RDMADCTL 0x0B   ///< Read Display MADCTL
#define ILI9341_RDPIXFMT 0x0C   ///< Read Display Pixel Format
#define ILI9341_RDIMGFMT 0x0D   ///< Read Display Image Format
#define ILI9341_RDSELFDIAG 0x0F ///< Read Display Self-Diagnostic Result

#define ILI9341_INVOFF 0x20   ///< Display Inversion OFF
#define ILI9341_INVON 0x21    ///< Display Inversion ON
#define ILI9341_GAMMASET 0x26 ///< Gamma Set
#define ILI9341_DISPOFF 0x28  ///< Display OFF
#define ILI9341_DISPON 0x29   ///< Display ON

#define ILI9341_CASET 0x2A ///< Column Address Set
#define ILI9341_PASET 0x2B ///< Page Address Set
#define ILI9341_RAMWR 0x2C ///< Memory Write
#define ILI9341_RAMRD 0x2E ///< Memory Read

#define ILI9341_PTLAR 0x30    ///< Partial Area
#define ILI9341_VSCRDEF 0x33  ///< Vertical Scrolling Definition
#define ILI9341_MADCTL 0x36   ///< Memory Access Control
#define ILI9341_VSCRSADD 0x37 ///< Vertical Scrolling Start Address
#define ILI9341_PIXFMT 0x3A   ///< COLMOD: Pixel Format Set

#define ILI9341_FRMCTR1 0xB1 ///< Frame Rate Control (In Normal Mode/Full Colors)
#define ILI9341_FRMCTR2 0xB2 ///< Frame Rate Control (In Idle Mode/8 colors)
#define ILI9341_FRMCTR3 0xB3 ///< Frame Rate control (In Partial Mode/Full Colors)
#define ILI9341_INVCTR 0xB4  ///< Display Inversion Control
#define ILI9341_DFUNCTR 0xB6 ///< Display Function Control

#define ILI9341_PWCTR1 0xC0 ///< Power Control 1
#define ILI9341_PWCTR2 0xC1 ///< Power Control 2
#define ILI9341_PWCTR3 0xC2 ///< Power Control 3
#define ILI9341_PWCTR4 0xC3 ///< Power Control 4
#define ILI9341_PWCTR5 0xC4 ///< Power Control 5
#define ILI9341_VMCTR1 0xC5 ///< VCOM Control 1
#define ILI9341_VMCTR2 0xC7 ///< VCOM Control 2

#define ILI9341_RDID1 0xDA ///< Read ID 1
#define ILI9341_RDID2 0xDB ///< Read ID 2
#define ILI9341_RDID3 0xDC ///< Read ID 3
#define ILI9341_RDID4 0xDD ///< Read ID 4

#define ILI9341_GMCTRP1 0xE0 ///< Positive Gamma Correction
#define ILI9341_GMCTRN1 0xE1 ///< Negative Gamma Correction

#define MADCTL_MY 0x80  ///< Bottom to top
#define MADCTL_MX 0x40  ///< Right to left
#define MADCTL_MV 0x20  ///< Reverse Mode
#define MADCTL_ML 0x10  ///< LCD refresh Bottom to top
#define MADCTL_RGB 0x00 ///< Red-Green-Blue pixel order
#define MADCTL_BGR 0x08 ///< Blue-Green-Red pixel order
#define MADCTL_MH 0x04  ///< LCD refresh right to left

typedef struct {
    uint8_t cmd;
    uint8_t databytes; //No of data in data; 0xFF = end of cmds.
    uint8_t delay;
    uint8_t data[16];
} lcd_init_cmd_t;

static const lcd_init_cmd_t st_init_cmds[] = { //
        { ILI9341_SWRESET, 0, 150, { 0 } },            //Soft Reset
        { ILI9341_DISPOFF, 0, 0, { 0 } },       //Display Off
        { ILI9341_PIXFMT, 1, 0, { 0x55 } },      //Pixel read=565, write=565.
        { ILI9341_PWCTR1, 1, 0, { 0x23 } },      //
        { ILI9341_PWCTR2, 1, 0, { 0x10 } },      //
        { ILI9341_VMCTR1, 2, 0, { 0x3E, 0x28 } },      //
        { ILI9341_VMCTR2, 1, 0, { 0x86 } },      //
        { ILI9341_MADCTL, 1, 0, { 0 } },      //
        { ILI9341_FRMCTR1, 2, 0, { 0x00, 0x18 } },      //
        { ILI9341_DFUNCTR, 4, 0, { 0x0A, 0xA2, 0x27, 0x04 } },      //
        { ILI9341_GAMMASET, 1, 0, { 0x01 } },      //
        { ILI9341_GMCTRP1, 15, 0, { 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09,
                                    0x00 } },      //
        { ILI9341_GMCTRN1, 15, 0, { 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36,
                                    0x0F } },      //
        { ILI9341_SLPOUT, 0, 150, { 0 } },          //Sleep Out
        { ILI9341_DISPON, 0, 0, { 0 } },           //Display On

        /*
         //
         { 0xEF, 3, 0, { 0x03, 0x80, 0x02 } },             //
         { 0xCF, 3, 0, { 0x00, 0xC1, 0x30 } }, //
         { 0xED, 4, 0, { 0x64, 0x03, 0x12, 0x81 } }, //
         { 0xE8, 3, 0, { 0x85, 0x00, 0x78 } }, //
         { 0xCB, 5, 0, { 0x39, 0x2C, 0x00, 0x34, 0x02 } }, //
         { 0xF7, 1, 0, { 0x20 } }, //
         { 0xEA, 2, 0, { 0x00, 0x00 } },  //
         { ILI9341_PWCTR1, 1, 0, { 0x23 } },             // Power control VRH[5:0]
         { ILI9341_PWCTR2, 1, 0, { 0x10 } },             // Power control SAP[2:0];BT[3:0]
         { ILI9341_VMCTR1, 2, 0, { 0x3e, 0x28 } },       // VCM control
         { ILI9341_VMCTR2, 1, 0, { 0x86 } },             // VCM control2
         { ILI9341_MADCTL, 1, 0, { 0 } },             // Memory Access Control
         { ILI9341_VSCRSADD, 1, 0, { 0x00 } },             // Vertical scroll zero
         { ILI9341_PIXFMT, 1, 0, { 0x55 } }, //
         { ILI9341_FRMCTR1, 2, 0, { 0x00, 0x18 } }, //
         { ILI9341_DFUNCTR, 3, 0, { 0x08, 0x82, 0x27 } }, // Display Function Control
         { 0xF2, 1, 0x00, { 0 } },                         // 3Gamma Function Disable
         { ILI9341_GAMMASET, 1, 0, { 0x01 } },             // Gamma curve selected
         { ILI9341_GMCTRP1, 15, 0, { 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, // Set Gamma
         0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00 } }, //
         { ILI9341_GMCTRN1, 15, 0, { 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, // Set Gamma
         0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F } }, //
         { ILI9341_SLPOUT, 0, 120, { 0 } },               // Exit Sleep
         { ILI9341_DISPON, 0, 120, { 0 } },               // Display on
         */
        { 0, 0xff, 0, { 0 } } // Stop

        };

void dgx_spi_ili9341_destroy_screen(dgx_screen_t *scr) {
    free(scr->draw_buffer);
    free(scr->trans);
}

//Initialize the display
esp_err_t dgx_spi_ili9341_init(dgx_screen_t *scr, gpio_num_t rst, uint8_t color_bits, dgx_color_order_t cbo) {
    esp_err_t ret = spi_bus_add_device(scr->host_id, &scr->devcfg, &scr->spi);
    if (ret != ESP_OK) return ret;
    scr->width = 240;
    scr->height = 320;
    scr->color_bits = color_bits;
    scr->lcd_ram_bits = scr->color_bits > 16 ? 24 : 16;
    scr->dir_x = LeftRight;
    scr->dir_y = TopBottom;
    scr->swap_xy = false;
    scr->rgb_order = cbo;
    scr->fb_col_hi = 0x0000;
    scr->fb_col_lo = 0x0000;
    scr->fb_row_hi = 0x0000;
    scr->fb_row_lo = 0x0000;
    scr->draw_buffer_len = 4000;
    scr->draw_buffer = heap_caps_calloc(1, scr->draw_buffer_len, MALLOC_CAP_8BIT);
    if (!scr->draw_buffer) return ESP_FAIL;
    scr->destroy = dgx_spi_ili9341_destroy_screen;
    scr->frame_buffer = 0;
    scr->trans[0].tx_data[0] = ILI9341_CASET;           //Column Address Set
    scr->trans[2].tx_data[0] = ILI9341_PASET;           //Page address set
    scr->trans[4].tx_data[0] = ILI9341_RAMWR;           //memory write

    const lcd_init_cmd_t *lcd_init_cmds = st_init_cmds;

//Initialize non-SPI GPIOs
    gpio_pad_select_gpio(rst);
    gpio_set_direction(rst, GPIO_MODE_OUTPUT);

//Reset the display
    gpio_set_level(rst, 1);
    vTaskDelay(100 / portTICK_RATE_MS);
    gpio_set_level(rst, 0);
    vTaskDelay(100 / portTICK_RATE_MS);
    gpio_set_level(rst, 1);

//Send all the commands
    uint8_t adj_data[1];
    for (uint8_t cidx = 0; lcd_init_cmds[cidx].databytes != 0xff; ++cidx) {
        uint8_t cmd = lcd_init_cmds[cidx].cmd;
        dgx_spi_sync_send_cmd(scr, cmd);
        uint8_t const *data = lcd_init_cmds[cidx].data;
        if (cmd == ILI9341_MADCTL) {
            if (cbo == BGR)
                *adj_data = *data & ~MADCTL_BGR;
            else
                *adj_data = *data | MADCTL_BGR;
            data = adj_data;
        } else if (cmd == ILI9341_PIXFMT) {
            data = adj_data;
            if (color_bits == 18)
                *adj_data = 0x66;
            else
                *adj_data = 0x55;
        }
        dgx_spi_sync_send_data(scr, data, lcd_init_cmds[cidx].databytes);
        if (lcd_init_cmds[cidx].delay) {
            int ms = lcd_init_cmds[cidx].delay == 255 ? 500 : lcd_init_cmds[cidx].delay;
            vTaskDelay(ms / portTICK_PERIOD_MS);
        }
    }
    return ESP_OK;
}

void dgx_spi_ili9341_orientation(dgx_screen_t *scr, dgx_orientation_t dir_x, dgx_orientation_t dir_y, bool swap_xy) {
    uint8_t cmd = ILI9341_MADCTL;
    uint8_t data[1] = { 0 };
    if (dir_x == RightLeft) data[0] |= MADCTL_MX;
    if (dir_y == BottomTop) data[0] |= MADCTL_MY;
    if (swap_xy) {
        data[0] |= MADCTL_MV;
        scr->width = 320;
        scr->height = 240;
    }
    if (scr->rgb_order == RGB) data[0] |= MADCTL_BGR;
    scr->dir_x = dir_x;
    scr->dir_y = dir_y;
    scr->swap_xy = swap_xy;
    dgx_spi_sync_send_cmd(scr, cmd);
    dgx_spi_sync_send_data(scr, data, 1);
}
