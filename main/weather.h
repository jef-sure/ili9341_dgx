#ifndef MAIN_WEATHER_H_
#define MAIN_WEATHER_H_

#include <stdint.h>

typedef struct weather_ {
    int idCond;
    const char *mainGroup;
    const char *description;
    int iconId;
    uint32_t symCode, symNight;
} weather_t;

#define WEATHERCOLOR24TORGB(r,g,b, color) \
    do {\
        uint32_t lclr = (uint32_t)(color);\
        (r) = (lclr >> 16) & 0xFF;\
        (g) = (lclr >> 8) & 0xFF;\
        (b) = lclr & 0xFF;\
    } while(0)

#define WEATHERRGBTOCOLOR24(r,g,b) (((r) << 16) | ((g) << 8) | (b))

const weather_t* findWeather(int id);
uint32_t findWeatherSymbol(int id, uint8_t is_daytime);
uint32_t weatherTempToRGB(int temp);

#endif /* MAIN_WEATHER_H_ */
