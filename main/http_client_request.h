#ifndef HTTP_CLIENT_REQUEST_H_
#define HTTP_CLIENT_REQUEST_H_

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "str.h"

#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

typedef struct http_client_response_ {
    str_t *body;
}http_client_response_t;

http_client_response_t* http_client_request_get(const char *url);

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on
#endif

#endif /* HTTP_CLIENT_REQUEST_H_ */
