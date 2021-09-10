/*
 * tzones.h
 *
 *  Created on: Mar 26, 2021
 *      Author: anton
 */

#ifndef _TZONES_H_
#define _TZONES_H_
#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

#include <stdbool.h>

typedef struct TZName_ {
    const char *name, *tzname;
}TZName_t;

const TZName_t* findTZ(const char *name);
bool setupTZ(const char *name);

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on
#endif
#endif /* _TZONES_H_ */
