/*
 * MIT License
 *
 * Copyright (c) 2021 Anton Petrusevich
 *
 */

#include <math.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_heap_caps_init.h"

#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#include <string.h>
#include <sys/time.h>

#include "lwip/err.h"
#include "lwip/sys.h"
#include "cJSON.h"

#include "dgx_spi_screen.h"
#include "drivers/spi_ili9341.h"
#include "dgx_draw.h"
#include "dgx_v_screen.h"
#include "dgx_interscreen.h"

#include "fonts/UbuntuCondensedRegular36.h"
#include "fonts/UbuntuCondensedRegular32.h"
#include "fonts/UbuntuCondensedRegular28.h"
#include "fonts/UbuntuCondensedRegular24.h"
#include "fonts/UbuntuCondensedRegular20.h"
#include "fonts/UbuntuCondensedRegular16.h"
#include "fonts/WeatherIconsRegular40.h"

#include "tzones.h"
#include "str.h"
#include "auto_location.h"
#include "weather.h"
#include "http_client_request.h"

#define DEFAULT_SSID CONFIG_ESP_WIFI_SSID
#define DEFAULT_PWD CONFIG_ESP_WIFI_PASSWORD
#define DEFAULT_SCAN_METHOD WIFI_ALL_CHANNEL_SCAN
#define DEFAULT_SORT_METHOD WIFI_CONNECT_AP_BY_SIGNAL
#define DEFAULT_AUTHMODE WIFI_AUTH_WPA2_PSK
#define DEFAULT_RSSI -127
#define WEATHER_API_KEY CONFIG_WEATHER_API_KEY

/* */
static const char *TAG = "WiFi Scan";

bool is_got_ip = false;

uint8_t Brightness = 100;

SemaphoreHandle_t xSemLocation = NULL;
WLocation_t Location;

typedef struct {
    int16_t x;
    int16_t y;
} point_t;

typedef enum message_type_ {
    ClockDigits, CityName, Weather
} message_type_t;

typedef struct task_message_ {
    message_type_t mType;
    uint8_t udigits;
} task_message_t;

SemaphoreHandle_t xSemVscr;
QueueHandle_t xQueDMS;

#define BC_SPACE 10
#define BC_MINUS 11

point_t digitCurves[12][3][4] = { //
        { // 0
        { { 10, 80 }, //
          { 5, -9 }, //
          { 95, -9 }, //
          { 90, 80 }, }, //
        { { 90, 80 }, //
          { 95, 170 }, //
          { 5, 170 }, //
          { 10, 80 }, }, //
        { { 85, 45 }, //
          { 62, 68 }, //
          { 38, 92 }, //
          { 15, 115 }, }, //
        },
        { // 1
        { { 10, 60 }, //
          { 20, 50 }, //
          { 35, 40 }, //
          { 50, 10 } }, //
        { { 50, 10 }, //
          { 50, 40 }, //
          { 50, 80 }, //
          { 50, 160 } }, //
        { { 10, 160 }, //
          { 30, 160 }, //
          { 50, 160 }, //
          { 90, 160 } }, //
        },
        { // 2
        { { 10, 60 }, //
          { 10, 5 }, //
          { 100, 5 }, //
          { 100, 60 } }, //
        { { 100, 60 }, //
          { 100, 100 }, //
          { 10, 105 }, //
          { 10, 160 } }, //
        { { 10, 160 }, //
          { 15, 160 }, //
          { 55, 160 }, //
          { 95, 160 } }, //
        },
        { // 3
        { { 20, 20 }, //
          { 50, -10 }, //
          { 150, 40 }, //
          { 50, 80 } }, //
        { { 50, 80 }, //
          { 100, 100 }, //
          { 100, 110 }, //
          { 95, 130 } }, //
        { { 95, 130 }, //
          { 85, 160 }, //
          { 50, 160 }, //
          { 20, 140 } }, //
        },
        { // 4
        { { 40, 10 }, //
          { 10, 100 }, //
          { 0, 105 }, //
          { 100, 100 } }, //
        { { 100, 100 }, //
          { 60, 105 }, //
          { 70, 50 }, //
          { 70, 30 } }, //
        { { 70, 30 }, //
          { 70, 70 }, //
          { 70, 110 }, //
          { 70, 160 } }, //
        },
        { // 5
        { { 25, 10 }, //
          { 20, 80 }, //
          { 25, 95 }, //
          { 30, 80 } }, //
        { { 30, 80 }, //
          { 100, 30 }, //
          { 100, 200 }, //
          { 20, 150 } }, //
        { { 25, 10 }, //
          { 45, 11 }, //
          { 70, 12 }, //
          { 90, 10 } }, //
        },
        { // 6
        { { 80, 20 }, //
          { 70, 10 }, //
          { 10, 90 }, //
          { 10, 110 } }, //
        { { 10, 110 }, //
          { 0, 160 }, //
          { 90, 160 }, //
          { 80, 110 } }, //
        { { 80, 110 }, //
          { 65, 70 }, //
          { 40, 70 }, //
          { 10, 110 } }, //
        },
        { // 7
        { { 10, 15 }, //
          { 5, 10 }, //
          { 100, 10 }, //
          { 90, 15 } }, //
        { { 90, 15 }, //
          { 65, 60 }, //
          { 47, 110 }, //
          { 30, 160 } }, //
        { { 80, 90 },  //
          { 60, 90 }, //
          { 40, 90 }, //
          { 25, 90 }, //
        }, //
        },
        { // 8
        { { 30, 50 }, //
          { -30, -10 }, //
          { 140, -5 }, //
          { 50, 80 } }, //
        { { 50, 80 }, //
          { -40, 170 }, //
          { 130, 180 }, //
          { 70, 110 } }, //
        { { 70, 110 }, //
          { 57, 90 }, //
          { 43, 70 }, //
          { 30, 50 }, //
        }//
        },
        { // 9
        { { 20, 50 }, //
          { 10, -5 }, //
          { 100, -5 }, //
          { 90, 50 }, //
        },//
        { { 90, 50 }, //
          { 90, 70 }, //
          { 30, 170 }, //
          { 20, 160 }, //
        },//
        { { 90, 50 }, //
          { 60, 90 }, //
          { 35, 90 }, //
          { 20, 50 }, //
        }, //
        },//
        { // empty
        { { 50, 80 }, //
          { 50, 80 }, //
          { 50, 80 }, //
          { 50, 80 }, //
        },//
        { { 50, 80 }, //
          { 50, 80 }, //
          { 50, 80 }, //
          { 50, 80 }, //
        },//
        { { 50, 80 }, //
          { 50, 80 }, //
          { 50, 80 }, //
          { 50, 80 }, //
        }, //
        },//
        { // minus
        { { 40, 80 }, //
          { 45, 80 }, //
          { 50, 80 }, //
          { 55, 80 }, //
        },//
        { { 55, 80 }, //
          { 60, 80 }, //
          { 65, 80 }, //
          { 70, 80 }, //
        },//
        { { 70, 80 }, //
          { 75, 80 }, //
          { 80, 80 }, //
          { 85, 80 }, //
        }, //
        }, //
        };

point_t symbolC[2][4] = { // 0
        { { 90, 60 }, //
          { 95, 30 }, //
          { 25, 30 }, //
          { 30, 100 }, //
        },//
        { { 30, 100 }, //
          { 25, 170 }, //
          { 95, 170 }, //
          { 90, 140 }, //
        }, //
        };

dgx_screen_t scrReal;
uint16_t LUT[256];
uint16_t weatherLUT[256];
dgx_screen_t vScrTime[8];
dgx_screen_t vScrCity;
dgx_screen_t vScrTemp[3];
dgx_screen_t vScrWeather;
dgx_screen_t vScrCelsius;

typedef struct morping_digit_ {
    dgx_screen_t *scr;
    uint8_t from;
    uint8_t to;
    point_t shift;
} morping_digit_t;

morping_digit_t mDigits[6] = { //
        { &vScrTime[0], 0, 0, { 0, 0 } }, //
        { &vScrTime[1], BC_SPACE, 0, { 0, 0 } }, //
        { &vScrTime[2], BC_SPACE, 0, { 0, 0 } }, //
        { &vScrTime[3], BC_SPACE, 0, { 0, 0 } }, //
        { &vScrTime[4], BC_SPACE, 0, { 0, 0 } }, //
        { &vScrTime[5], BC_SPACE, BC_SPACE, { 0, 0 } }, //
        };

morping_digit_t mTempDigits[3] = { //
        { &vScrTemp[0], BC_SPACE, BC_SPACE, { 0, 0 } }, //
        { &vScrTemp[1], BC_SPACE, BC_SPACE, { 0, 0 } }, //
        { &vScrTemp[2], BC_SPACE, BC_SPACE, { 0, 0 } }, //
        };

typedef struct virt_screen_ {
    dgx_screen_t *vscr;
    dgx_screen_t *dscr;
    point_t shift;
} virt_screen_t;

virt_screen_t vTimeDigits[8] = { //
        { &vScrTime[0], &scrReal, { 0, 0 } }, //
        { &vScrTime[1], &scrReal, { 40, 0 } }, //
        { &vScrTime[2], &scrReal, { 120, 0 } }, //
        { &vScrTime[3], &scrReal, { 160, 0 } }, //
        { &vScrTime[4], &scrReal, { 240, 0 } }, //
        { &vScrTime[5], &scrReal, { 280, 0 } }, //
        { &vScrTime[6], &scrReal, { 80, 0 } }, //
        { &vScrTime[7], &scrReal, { 200, 0 } }, //
        };

virt_screen_t vTempDigits[5] = { //
        { &vScrTemp[0], &scrReal, { 80, 120 } }, //
        { &vScrTemp[1], &scrReal, { 112, 120 } }, //
        { &vScrTemp[2], &scrReal, { 144, 120 } }, //
        { &vScrWeather, &scrReal, { 20, 120 } }, //
        { &vScrCelsius, &scrReal, { 176, 120 } }, //
        };

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        is_got_ip = false;
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        is_got_ip = true;
    }
}

static void fast_scan() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT()
    ;
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

    // Initialize default station as network interface instance (esp-netif)
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    // Initialize and start WiFi
    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));
    strcpy((char*) wifi_config.sta.ssid, DEFAULT_SSID);
    strcpy((char*) wifi_config.sta.password, DEFAULT_PWD);
    wifi_config.sta.scan_method = DEFAULT_SCAN_METHOD;
    wifi_config.sta.sort_method = DEFAULT_SORT_METHOD;
    wifi_config.sta.threshold.rssi = DEFAULT_RSSI;
    wifi_config.sta.threshold.authmode = DEFAULT_AUTHMODE;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

point_t cubic_bezier_point(float t, const point_t *points) {
    point_t ret;
    float cX = 3 * (points[1].x - points[0].x);
    float bX = 3 * (points[2].x - points[1].x) - cX;
    float aX = points[3].x - points[0].x - cX - bX;
    float cY = 3 * (points[1].y - points[0].y);
    float bY = 3 * (points[2].y - points[1].y) - cY;
    float aY = points[3].y - points[0].y - cY - bY;
    ret.x = ((aX * pow(t, 3)) + (bX * pow(t, 2)) + (cX * t) + points[0].x) + 0.499f;
    ret.y = ((aY * pow(t, 3)) + (bY * pow(t, 2)) + (cY * t) + points[0].y) + 0.499f;
    return ret;
}

point_t linear_move(float t, const point_t p0, const point_t p1) {
    point_t ret;
    float dX = t * (p1.x - p0.x);
    float dY = t * (p1.y - p0.y);
    ret.x = dX + p0.x;
    ret.y = dY + p0.y;
    return ret;
}

point_t affine_transform(float scale_x, float scale_y, point_t shift, float t, const point_t p0, const point_t p1) {
    point_t ret;
    float dX = t * (p1.x - p0.x);
    float dY = t * (p1.y - p0.y);
    ret.x = scale_x * (dX + p0.x) + shift.x;
    ret.y = scale_y * (dY + p0.y) + shift.y;
    return ret;
}

#define BZACCURACY 13

void cubic_bezier(dgx_screen_t *scr, uint32_t color, const point_t *points, int thikness) {
    point_t ps;
    point_t pe;
    ps.x = points[0].x;
    ps.y = points[0].y;
    if (ps.x == points[1].x && ps.x == points[2].x && ps.x == points[3].x && ps.y == points[1].y && ps.y == points[2].y
            && ps.y == points[3].y) return;
    for (int i = 0; i <= BZACCURACY; i++) {
        pe = cubic_bezier_point((float) i / BZACCURACY, points);
        if (thikness > 1) {
            dgx_draw_line_thick(scr, ps.x, ps.y, pe.x, pe.y, thikness, color);
        } else {
            dgx_draw_line(scr, ps.x, ps.y, pe.x, pe.y, color);
        }
        if (thikness > 5) {
            dgx_draw_circle_solid(scr, pe.x, pe.y, (thikness - 1) / 2, color);
        }
        ps = pe;
    }
}

#define PGET(val) ((val) == (uint8_t)0xff? (uint16_t)0xff: (uint16_t)0)
void antialias_vscreen8(dgx_screen_t *vscr) {
    const int16_t margin = 1;
    const int16_t h = vscr->height - margin;
    const int16_t width = vscr->width;
    const int16_t w = width - margin;

    for (int16_t row = margin; row < h; ++row) {
        uint8_t *pb = vscr->frame_buffer + width * row + margin;
        for (int16_t col = margin; col < w; ++col, ++pb) {
            if (*pb == 0) {
                uint16_t acc = (PGET(pb[-1]) + PGET(pb[1]) + PGET(pb[-width]) + PGET(pb[width])) / 4;
                *pb = acc;
            }
        }
    }
}

uint8_t morphing_digits(morping_digit_t *mD, int mDNum, float t, uint32_t color, int thickness) {
    float scale_x = mD[0].scr->width / 120.0;
    float scale_y = mD[0].scr->height / 170.0;
    uint8_t udigits = 0;
    for (uint8_t di = 0; di < mDNum; ++di) {
        morping_digit_t *md = mD + di;
        if (md->from == md->to) {
            continue;
        }
        udigits |= 1 << di;
        dgx_fill_rectangle(md->scr, 0, 0, md->scr->width, md->scr->height, 0);
        point_t curve[3][4];
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 4; ++j) {
                curve[i][j] = affine_transform(scale_x, scale_y, md->shift, t, digitCurves[md->from][i][j], digitCurves[md->to][i][j]);
            }
        }
        for (int i = 0; i < 3; ++i) {
            cubic_bezier(md->scr, color, curve[i], thickness);
        }
        antialias_vscreen8(md->scr);
    }
    return udigits;
}

void drawCelsius(virt_screen_t *vscr) {
    float scale_x = vscr->vscr->width / 120.0;
    float scale_y = vscr->vscr->height / 170.0;
    dgx_fill_rectangle(vscr->vscr, 0, 0, vscr->vscr->width, vscr->vscr->height, 0);
    point_t curve[2][4];
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 4; ++j) {
            point_t t = symbolC[i][j];
            t.x *= scale_x;
            t.y *= scale_y;
            curve[i][j] = t;
        }
    }
    for (int i = 0; i < 2; ++i) {
        cubic_bezier(vscr->vscr, 255, curve[i], 3);
    }
    dgx_draw_circle(vscr->vscr, 12, 12, 5, 255);
    antialias_vscreen8(vscr->vscr);
}

void vTaskDigitMorphing(void *pvParameters) {
    static task_message_t message = { .mType = ClockDigits };
    static task_message_t *sndval = &message;
    uint32_t color = 0xff;

    for (;;) {
        struct timeval now;
        gettimeofday(&now, 0);
        if (now.tv_usec < 500000) {
            time_t delay = (500000 - now.tv_usec) / 1000 / portTICK_PERIOD_MS;
            vTaskDelay(delay);
            now.tv_sec++;
        }
        struct tm timeinfo;
        localtime_r(&now.tv_sec, &timeinfo);
        mDigits[5].to = timeinfo.tm_sec % 10;
        mDigits[4].to = timeinfo.tm_sec / 10;
        mDigits[3].to = timeinfo.tm_min % 10;
        mDigits[2].to = timeinfo.tm_min / 10;
        mDigits[1].to = timeinfo.tm_hour % 10;
        mDigits[0].to = timeinfo.tm_hour / 10;
        float t = 0;
        const int64_t target_interval = 500000ll;
        int64_t start_time = esp_timer_get_time();
        while (t <= 1) {
            int64_t st = esp_timer_get_time();
            t = ((st - start_time) / (float) target_interval);
            if (t > 1) {
                t = 1;
            }
            xSemaphoreTake(xSemVscr, 0);
            message.udigits = morphing_digits(mDigits, sizeof(mDigits) / sizeof(mDigits[0]), t, color, 5);
            int16_t r = fabs(1 - 2 * t) * 3 + 1;
            int16_t cx = vTimeDigits[6].vscr->width / 2;
            int16_t cy1 = vTimeDigits[6].vscr->height / 3;
            int16_t cy2 = 2 * (vTimeDigits[6].vscr->height / 3);
            dgx_fill_rectangle(vTimeDigits[6].vscr, 0, 0, vTimeDigits[6].vscr->width, vTimeDigits[6].vscr->height, 0);
            dgx_draw_circle(vTimeDigits[6].vscr, cx, cy1, r, color);
            dgx_draw_circle(vTimeDigits[6].vscr, cx, cy2, r, color);
            dgx_fill_rectangle(vTimeDigits[7].vscr, 0, 0, vTimeDigits[7].vscr->width, vTimeDigits[7].vscr->height, 0);
            dgx_draw_circle(vTimeDigits[7].vscr, cx, cy1, r, color);
            dgx_draw_circle(vTimeDigits[7].vscr, cx, cy2, r, color);
            antialias_vscreen8(vTimeDigits[6].vscr);
            antialias_vscreen8(vTimeDigits[7].vscr);
            message.udigits |= 0xc0;
            xSemaphoreGive(xSemVscr);
            xQueueSendToFront(xQueDMS, &sndval, 0);
            taskYIELD();
            if (t == 1) break;
            int64_t et = esp_timer_get_time();
            if (et - st < 33333 /* 1/30 */) vTaskDelay((33333 - (et - st)) / 1000 / portTICK_PERIOD_MS);
        }
        for (int i = 0; i < 6; ++i)
            mDigits[i].from = mDigits[i].to;
    }
}

void write_city(dgx_screen_t *scr, const char *name) {
    const uint32_t city_color = 0xff;
    dgx_font_t *tf[] = { //
            UbuntuCondensedRegular36(), //
            UbuntuCondensedRegular32(), //
            UbuntuCondensedRegular28(), //
            UbuntuCondensedRegular24(), //
            UbuntuCondensedRegular20(), //
            UbuntuCondensedRegular16(), //
            };
    for (size_t i = 0; i < sizeof(tf) / sizeof(tf[0]); ++i) {
        if (tf[i]->yAdvance > scr->height) continue;
        int16_t ycorner;
        int16_t height;
        int16_t width = dgx_font_string_bounds(name, tf[i], &ycorner, &height);
        int16_t x = scr->width / 2 - width / 2;
        if (x >= 0 || i + 1 == sizeof(tf) / sizeof(tf[0])) {
            if (x < 0) x = 0;
            dgx_font_string_utf8_screen(scr, x, scr->height - 3, name, city_color, 0, LeftRight, TopBottom, false, tf[i]);
            antialias_vscreen8(scr);
            return;
        }
    }
}

void vTaskGetLocation(void *pvParameters) {
    static task_message_t message = { .mType = CityName, .udigits = 0 };
    static task_message_t *sndval = &message;
    const long delay = 600000l;
    for (;;) {
        if (!is_got_ip) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }
        WLocation_t location = auto_location();
        if (location.timezone) {
            xSemaphoreTake(xSemLocation, 0);
            if (!Location.timezone || strcmp(str_c(Location.timezone), str_c(location.timezone)) != 0) {
                setupTZ(str_c(location.timezone));
            }
            write_city(&vScrCity, str_c(location.city));
            free(Location.country);
            free(Location.countryCode);
            free(Location.region);
            free(Location.regionName);
            free(Location.city);
            free(Location.zip);
            free(Location.timezone);
            free(Location.isp);
            free(Location.ip);
            Location = location;
            xSemaphoreGive(xSemLocation);
            xQueueSendToFront(xQueDMS, &sndval, 0);
        } else {
            vTaskDelay(10000 / portTICK_PERIOD_MS);
            continue;
        }
        vTaskDelay(delay / portTICK_PERIOD_MS);
    }
}

void vTaskGetWeather(void *pvParameters) {
    const long delay = 60000l;
    static task_message_t message = { .mType = Weather, .udigits = 0 };
    static task_message_t *sndval = &message;
    dgx_font_t *weather_icon_font = WeatherIconsRegular40();
    int storedTemp = 100500;
    bool is_got_location = false;
    for (;;) {
        if ( xSemaphoreTake( xSemLocation, 100 / portTICK_PERIOD_MS ) == pdTRUE) {
            str_t *url = 0;
            if (Location.zip) {
                url = str_new_pc("https://api.openweathermap.org/data/2.5/weather?q=");
                str_append_str(&url, Location.zip);
                str_append_pc(&url, ",");
                str_append_str(&url, Location.countryCode);
                str_append_pc(&url, "&appid=" WEATHER_API_KEY "&units=metric");
                printf("weather url request: %s\n", str_c(url));
            }
            xSemaphoreGive(xSemLocation);
            if (url) {
                ESP_LOGI("getWeather", "starting");
                heap_caps_print_heap_info(MALLOC_CAP_8BIT);
                http_client_response_t *weather_json = http_client_request_get(str_c(url));
                ESP_LOGI("getWeather", "got: %p", weather_json);
                if (weather_json) {
                    cJSON *json = weather_json->body ? cJSON_Parse(str_c(weather_json->body)) : 0;
                    if (json) {
                        printf("weather_json: %s\n", str_c(weather_json->body));
                        int cod = cJSON_GetObjectItem(json, "cod")->valueint;
                        if (cod == 200) {
                            is_got_location = true;
                            cJSON *weather = cJSON_GetArrayItem(cJSON_GetObjectItem(json, "weather"), 0);
                            cJSON *jmain = cJSON_GetObjectItem(json, "main");
                            cJSON *jsys = cJSON_GetObjectItem(json, "sys");
                            int temp = cJSON_GetObjectItem(jmain, "temp")->valueint;
                            int weather_id = cJSON_GetObjectItem(weather, "id")->valueint;
                            int dt = cJSON_GetObjectItem(json, "dt")->valueint;
                            int sunrise = cJSON_GetObjectItem(jsys, "sunrise")->valueint;
                            int sunset = cJSON_GetObjectItem(jsys, "sunset")->valueint;
                            uint8_t is_daytime = dt <= sunset && dt >= sunrise;
                            uint32_t wsym = findWeatherSymbol(weather_id, is_daytime);
                            uint32_t color = 255;
                            int16_t xAdvance;
                            const glyph_t *wsym_glyph = dgx_font_find_glyph(wsym, weather_icon_font, &xAdvance);
                            int16_t wx = vScrWeather.width / 2 - xAdvance / 2;
                            int16_t wy = vScrWeather.height / 2 - wsym_glyph->height / 2 - wsym_glyph->yOffset - 3;
                            dgx_fill_rectangle(&vScrWeather, 0, 0, vScrWeather.width, vScrWeather.height, 0);
                            dgx_font_char_to_screen(&vScrWeather, wx, wy, wsym, color, 0, LeftRight, TopBottom, false, weather_icon_font);
                            antialias_vscreen8(&vScrWeather);
                            message.udigits = 1 << 3;
                            if (storedTemp != temp) {
                                uint32_t wc = weatherTempToRGB(temp);
                                uint8_t r;
                                uint8_t g;
                                uint8_t b;
                                WEATHERCOLOR24TORGB(r, g, b, wc);
//                            printf("WEATHERCOLOR24TORGB(%d, %d, %d, %d)\n", r, g, b, wc);
                                for (uint16_t var = 0; var < 256; ++var) {
                                    weatherLUT[var] = dgx_rgb_to_16(var * r / 255, var * g / 255, var * b / 255);
//                                printf("WweatherLUT[%d] = %d\n", var, weatherLUT[var]);
                                }
                                int absTemp = temp >= 0 ? temp : -temp;
                                mTempDigits[2].to = absTemp % 10;
                                if (absTemp > 9) {
                                    mTempDigits[1].to = (absTemp / 10) % 10;
                                    mTempDigits[0].to = (temp < 0) ? BC_MINUS : BC_SPACE;
                                } else {
                                    mTempDigits[1].to = (temp < 0) ? BC_MINUS : BC_SPACE;
                                    mTempDigits[0].to = BC_SPACE;
                                }
                                float t = 0;
                                const int64_t target_interval = 500000ll;
                                int64_t start_time = esp_timer_get_time();
                                while (t <= 1) {
                                    int64_t st = esp_timer_get_time();
                                    t = ((st - start_time) / (float) target_interval);
                                    if (t > 1) {
                                        t = 1;
                                    }
                                    xSemaphoreTake(xSemVscr, 0);
                                    message.udigits |= morphing_digits(mTempDigits, sizeof(mTempDigits) / sizeof(mTempDigits[0]), t, color, 3);
                                    xSemaphoreGive(xSemVscr);
                                    xQueueSendToFront(xQueDMS, &sndval, 0);
                                    taskYIELD();
                                    if (t == 1) break;
                                    int64_t et = esp_timer_get_time();
                                    if (et - st < 33333 /* 1/30 */) vTaskDelay((33333 - (et - st)) / 1000 / portTICK_PERIOD_MS);
                                    message.udigits = 0;
                                }
                                for (int i = 0; i < sizeof(mTempDigits) / sizeof(mTempDigits[0]); ++i)
                                    mTempDigits[i].from = mTempDigits[i].to;
                            } else {
                                xQueueSendToFront(xQueDMS, &sndval, 0);
                                taskYIELD();
                            }
                        }
                        cJSON_Delete(json);
                    } else {
                        printf("url %s returns: %s\n", str_c(url), str_c(weather_json->body));
                    }
                    free(weather_json->body);
                    free(weather_json);
                }
            }
            free(url);
        }
        if (is_got_location) {
            vTaskDelay(delay / portTICK_PERIOD_MS);
        } else {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
}

void app_main(void) {
    esp_err_t rc = dgx_spi_init(&scrReal, VSPI_HOST, 23, 18, -1, 16, 33 * 1000000);
    assert(rc == ESP_OK);
    rc = dgx_spi_ili9341_init(&scrReal, 17, 16, RGB);
    assert(rc == ESP_OK);
    xQueDMS = xQueueCreate(20, sizeof(task_message_t*));
    assert(xQueDMS != NULL);
    xSemVscr = xSemaphoreCreateBinary();
    assert(xSemVscr != NULL);
    xSemLocation = xSemaphoreCreateMutex();
    assert(xSemLocation != NULL);
    dgx_spi_ili9341_orientation(&scrReal, BottomTop, LeftRight, true);
    dgx_fill_rectangle(&scrReal, 0, 0, scrReal.width, scrReal.height, 0);
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    for (uint16_t var = 0; var < 256; ++var) {
        LUT[var] = dgx_rgb_to_16(var, var, var);
    }
    for (uint8_t s = 0; s < 8; ++s) {
        dgx_v_init(&vScrTime[s], scrReal.width / 8, 120, 8);
    }
    for (uint8_t s = 0; s < 3; ++s) {
        dgx_v_init(&vScrTemp[s], 32, 72, 8);
    }
    dgx_v_init(&vScrCity, scrReal.width, scrReal.height / 5, 8);
    dgx_v_init(&vScrWeather, 60, 72, 8);
    dgx_v_init(&vScrCelsius, 40, 72, 8);
//    adc1_config_width(ADC_WIDTH_BIT_12);
//    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
    fast_scan();
    bool sntp_inited = false;
    task_message_t *message;
    xTaskCreate(vTaskDigitMorphing, "DigiMorph", 4096, 0, tskIDLE_PRIORITY + 1, 0);
    xTaskCreate(vTaskGetLocation, "Location", 4096, 0, tskIDLE_PRIORITY + 1, 0);
    xTaskCreate(vTaskGetWeather, "Weather", 4096, 0, tskIDLE_PRIORITY + 1, 0);
    while (1) {
        if (!sntp_inited) {
            if (is_got_ip) {
                sntp_setoperatingmode(SNTP_OPMODE_POLL);
                sntp_setservername(0, "pool.ntp.org");
                sntp_init();
                sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
                sntp_inited = true;
            }
        }
        if (xQueueReceive(xQueDMS, &message, (TickType_t )100)) {
            if (message->mType == ClockDigits) {
                uint8_t udigits = message->udigits;
                xSemaphoreTake(xSemVscr, 0);
                for (uint8_t bi = 0; bi < 8; ++bi) {
                    if ((udigits & (1 << bi))) {
                        dgx_vscreen8_to_screen16(vTimeDigits[bi].dscr, vTimeDigits[bi].shift.x, vTimeDigits[bi].shift.y,
                                                 vTimeDigits[bi].vscr, LUT);
                    }
                }
                xSemaphoreGive(xSemVscr);
            } else if (message->mType == CityName) {
                dgx_vscreen8_to_screen16(&scrReal, 0, 192, &vScrCity, LUT);
            } else if (message->mType == Weather) {
                uint8_t udigits = message->udigits;
                xSemaphoreTake(xSemVscr, 0);
                drawCelsius(&vTempDigits[4]);
                udigits |= 1 << 4;
                for (uint8_t bi = 0; bi < sizeof(vTempDigits) / sizeof(vTempDigits[0]); ++bi) {
                    if ((udigits & (1 << bi))) {
                        dgx_vscreen8_to_screen16(vTempDigits[bi].dscr, vTempDigits[bi].shift.x, vTempDigits[bi].shift.y,
                                                 vTempDigits[bi].vscr, weatherLUT);
                    }
                }
                xSemaphoreGive(xSemVscr);
            }
        }
//        int val = adc1_get_raw(ADC1_CHANNEL_0);
//        if (val > 1000) {
//            Brightness = 100;
//        } else if (val < 100) {
//            Brightness = 10;
//        } else {
//            Brightness = val / 10;
//        }
    }
}

