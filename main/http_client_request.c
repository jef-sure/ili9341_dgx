/*
 * MIT License
 *
 * Copyright (c) 2021 Anton Petrusevich
 *
 */

#include "http_client_request.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include <sys/time.h>

static const char *TAG = "http_client_request";

static esp_err_t _http_collect_data_handle(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
        printf("%.*s", evt->data_len, (char*) evt->data);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        if (!esp_http_client_is_chunked_response(evt->client)) {
            if (evt->data && evt->data_len) {
                str_append_pcln(evt->user_data, (char*) evt->data, evt->data_len);
            }
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    }
    return ESP_OK;
}

http_client_response_t* http_client_request_get(const char *url) {
    http_client_response_t *ret = calloc(1, sizeof(http_client_response_t));
    assert(ret);
    esp_http_client_config_t config = { //
            .url = url, //
            .event_handler = _http_collect_data_handle, //
            .user_data = &ret->body, //
            .timeout_ms = 2000, //
            .is_async = url[4] == 's' ? true : false, //
            };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err;
    struct timeval now;
    gettimeofday(&now, 0);
    uint64_t ustart = now.tv_sec * 1000 + now.tv_usec / 1000;
    uint64_t unow = ustart;
    while (unow < ustart + 5000) {
        ESP_LOGI(TAG, "starting GET request for url %s", url);
        err = esp_http_client_perform(client);
        ESP_LOGI(TAG, "RC for url %d", err);
        if (err != ESP_ERR_HTTP_EAGAIN) {
            break;
        }
        gettimeofday(&now, 0);
        unow = now.tv_sec * 1000 + now.tv_usec / 1000;
    }
    ESP_LOGI(TAG, "cleaning after %s", url);
    esp_http_client_cleanup(client);
    if (err == ESP_OK) {
        if (ret->body) {
            ESP_LOGI(TAG, "got body: %s", str_c(ret->body));
        } else {
            ESP_LOGI(TAG, "got empty body");
        }
        return ret;
    }
    ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    free(ret->body);
    free(ret);
    return 0;
}
