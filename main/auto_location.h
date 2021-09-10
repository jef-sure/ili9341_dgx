#ifndef _AUTO_LOCATION_H_
#define _AUTO_LOCATION_H_

#include "str.h"

#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

typedef struct WLocation_ {
    str_t *country, *countryCode, *region, *regionName, *city, *zip, *timezone, *isp, *ip;
    float lat, lon;
}WLocation_t;

WLocation_t auto_location();

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on
#endif

#endif /* _AUTO_LOCATION_H_ */
