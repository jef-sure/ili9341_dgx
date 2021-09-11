/*
 * MIT License
 *
 * Copyright (c) 2021 Anton Petrusevich
 *
 */

#include "weather.h"
#include "math.h"

weather_t openweather[] = { //
        { 200, "Thunderstorm", "thunderstorm with light rain", 11, 0xf010, 0xf03b }, //
        { 201, "Thunderstorm", "thunderstorm with rain", 11, 0xf010, 0xf03b }, //
        { 202, "Thunderstorm", "thunderstorm with heavy rain", 11, 0xf010, 0xf03b }, //
        { 210, "Thunderstorm", "light thunderstorm", 11, 0xf010, 0xf03b }, //
        { 211, "Thunderstorm", "thunderstorm", 11, 0xf010, 0xf03b }, //
        { 212, "Thunderstorm", "heavy thunderstorm", 11, 0xf010, 0xf03b }, //
        { 221, "Thunderstorm", "ragged thunderstorm", 11, 0xf010, 0xf03b }, //
        { 230, "Thunderstorm", "thunderstorm with light drizzle", 11, 0xf010, 0xf03b }, //
        { 231, "Thunderstorm", "thunderstorm with drizzle", 11, 0xf010, 0xf03b }, //
        { 232, "Thunderstorm", "thunderstorm with heavy drizzle", 11, 0xf010, 0xf03b }, //
        { 300, "Drizzle", "light intensity drizzle", 9, 0xf008, 0xf036 }, //
        { 301, "Drizzle", "drizzle", 9, 0xf008, 0xf036 }, //
        { 302, "Drizzle", "heavy intensity drizzle", 9, 0xf00b, 0xf039 }, //
        { 310, "Drizzle", "light intensity drizzle rain", 9, 0xf00b, 0xf039 }, //
        { 311, "Drizzle", "drizzle rain", 9, 0xf00b, 0xf039 }, //
        { 312, "Drizzle", "heavy intensity drizzle rain", 9, 0xf008, 0xf036 }, //
        { 313, "Drizzle", "shower rain and drizzle", 9, 0xf009, 0xf037 }, //
        { 314, "Drizzle", "heavy shower rain and drizzle", 9, 0xf009, 0xf037 }, //
        { 321, "Drizzle", "shower drizzle", 9, 0xf009, 0xf037 }, //
        { 500, "Rain", "light rain", 10, 0xf009, 0xf037 }, //
        { 501, "Rain", "moderate rain", 10, 0xf009, 0xf037 }, //
        { 502, "Rain", "heavy intensity rain", 10, 0xf009, 0xf037 }, //
        { 503, "Rain", "very heavy rain", 10, 0xf009, 0xf037 }, //
        { 504, "Rain", "extreme rain", 10, 0xf009, 0xf037 }, //
        { 511, "Rain", "freezing rain", 13, 0xf0b2, 0xf0b3 }, //
        { 520, "Rain", "light intensity shower rain", 9, 0xf009, 0xf037 }, //
        { 521, "Rain", "shower rain", 9, 0xf009, 0xf037 }, //
        { 522, "Rain", "heavy intensity shower rain", 9, 0xf009, 0xf037 }, //
        { 531, "Rain", "ragged shower rain", 9, 0xf009, 0xf037 }, //
        { 600, "Snow", "light snow", 13, 0xf00a, 0xf038 }, //
        { 601, "Snow", "Snow", 13, 0xf00a, 0xf038 }, //
        { 602, "Snow", "Heavy snow", 13, 0xf00a, 0xf038 }, //
        { 611, "Snow", "Sleet", 13, 0xf0b2, 0xf0b3 }, //
        { 612, "Snow", "Light shower sleet", 13, 0xf0b2, 0xf0b3 }, //
        { 613, "Snow", "Shower sleet", 13, 0xf0b2, 0xf0b3 }, //
        { 615, "Snow", "Light rain and snow", 13, 0xf00a, 0xf038 }, //
        { 616, "Snow", "Rain and snow", 13, 0xf00a, 0xf038 }, //
        { 620, "Snow", "Light shower snow", 13, 0xf00a, 0xf038 }, //
        { 621, "Snow", "Shower snow", 13, 0xf00a, 0xf038 }, //
        { 622, "Snow", "Heavy shower snow", 13, 0xf00a, 0xf038 }, //
        { 701, "Mist", "mist", 50, 0xf003, 0xf014 }, //
        { 711, "Smoke", "Smoke", 50, 0xf062, 0xf062 }, //
        { 721, "Haze", "Haze", 50, 0xf0b6, 0xf0b6 }, //
        { 731, "Dust", "sand/ dust whirls", 50, 0xf063, 0xf063 }, //
        { 741, "Fog", "fog", 50, 0xf003, 0xf014 }, //
        { 751, "Sand", "sand", 50, 0xf082, 0xf082 }, //
        { 761, "Dust", "dust", 50, 0xf063, 0xf063 }, //
        { 762, "Ash", "volcanic ash", 50, 0xf0c8, 0xf0c8 }, //
        { 771, "Squall", "squalls", 50, 0xf021, 0xf021 }, //
        { 781, "Tornado", "tornado", 50, 0xf056, 0xf056 }, //
        { 800, "Clear", "clear sky", 01, 0xf00d, 0xf02e }, //
        { 801, "Clouds", "few clouds", 11, 0xf00c, 0xf083 }, //
        { 802, "Clouds", "scattered clouds", 25, 0xf00c, 0xf083 }, //
        { 803, "Clouds", "broken clouds", 51, 0xf002, 0xf031 }, //
        { 804, "Clouds", "overcast clouds", 85, 0xf002, 0xf031 }, //
        };

const weather_t* findWeather(int id) {
    weather_t *found = 0;
    for (int i = 0; i < sizeof(openweather) / sizeof(openweather[0]); ++i)
        if (openweather[i].idCond == id) {
            found = openweather + i;
            break;
        }
    if (found) {
        return found;
    }
    return 0;
}

uint32_t findWeatherSymbol(int id, uint8_t is_daytime) {
    const weather_t *found = findWeather(id);
    if (found) {
        if (found->symNight && !is_daytime) return found->symNight;
        return found->symCode;
    }
    return 0;
}

static uint8_t clamp(int rgb) {
    return rgb < 0 ? 0 : rgb > 255 ? 255 : rgb;
}

#define MAX(a,b) ((a) > (b)? (a): (b))

uint32_t weatherTempToRGB(int temp) {
    int r, g, b;
    if (temp >= 23) {
        float t = (temp - 23) / 27.0f;
        g = (1 - t) * 200;
        r = 200 + t * 50;
        b = 0;
    } else {
        float t = (23 - temp) / 33.0f;
        b = t * 200;
        g = sqrt((1 - t) >= 0 ? 1 - t : 0) * 200;
        r = 220 - sqrt(t / 2) * 200;
    }
    uint8_t mrgb = MAX(MAX(clamp(r), clamp(g)), clamp(b));
    float cs = 224.0f / mrgb;
    return WEATHERRGBTOCOLOR24(clamp(r * cs), clamp(g * cs), clamp(b * cs));
}

