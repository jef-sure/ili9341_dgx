#include "http_client_request.h"
#include "cJSON.h"
#include "auto_location.h"
#include "esp_system.h"

#define cpjs(field) \
    do {\
        cJSON *tmp = cJSON_GetObjectItem(json, #field);\
        if (tmp) ret.field = str_new_pc(tmp->valuestring);\
    } while(0)

#define cpjss(field, source) \
    do {\
        cJSON *tmp = cJSON_GetObjectItem(json, #source);\
        if (tmp) ret.field = str_new_pc(tmp->valuestring);\
    } while(0)

#define cpjsn(field, source, str) \
    do {\
        cJSON *tmp = cJSON_GetObjectItem(json, #source);\
        if (tmp) ret.field = tmp->value##str;\
    } while(0)

/*
 *
 *
 http_client_response_t *location_json = http_client_request_get("http://ip-api.com/json");
 // {"status":"success","country":"Germany","countryCode":"DE","region":"RP","regionName":"Rheinland-Pfalz","city":"Neustadt","zip":"67433","lat":49.3439,"lon":8.1509,"timezone":"Europe/Berlin","isp":"Deutsche Telekom AG","org":"Deutsche Telekom AG","as":"AS3320 Deutsche Telekom AG","query":"79.199.66.249"}
 if (location_json) {
 cJSON *json = cJSON_Parse(str_c(location_json->body));
 if (json) {
 cpjs(country);
 cpjs(countryCode);
 cpjs(region);
 cpjs(regionName);
 cpjs(city);
 cpjs(zip);
 cpjs(timezone);
 cpjs(isp);
 cpjss(ip, query);
 cpjsn(lat, lat, double);
 cpjsn(lon, lon, double);
 cJSON_Delete(json);
 }
 }
 *
 *
 */

#define IPINFO_URL(TOKEN) "https://ipinfo.io?token=" TOKEN

WLocation_t auto_location() {
    WLocation_t ret;
    memset(&ret, 0, sizeof(ret));
    http_client_response_t *location_json = http_client_request_get(IPINFO_URL(CONFIG_ESP_IPINFO_TOKEN));
    // {
//    "ip": "79.199.90.216",
//    "hostname": "p4fc75ad8.dip0.t-ipconnect.de",
//    "city": "Neustadt",
//    "region": "Rheinland-Pfalz",
//    "country": "DE",
//    "loc": "49.3501,8.1389",
//    "org": "AS3320 Deutsche Telekom AG",
//    "postal": "67434",
//    "timezone": "Europe/Berlin"
//  }
    if (location_json) {
        cJSON *json = cJSON_Parse(str_c(location_json->body));
        if (json) {
            cpjss(countryCode, country);
            cpjss(regionName, region);
            cpjs(city);
            cpjss(zip, postal);
            cpjs(timezone);
            cpjss(isp, org);
            cpjs(ip);
            cJSON_Delete(json);
        }
    }
    if (location_json) {
        free(location_json->body);
        free(location_json);
    }
    return ret;
}
