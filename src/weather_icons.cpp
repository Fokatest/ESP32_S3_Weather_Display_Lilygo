#include "weather_icons.h"
#include <math.h>

uint16_t COLOR_BG;
uint16_t COLOR_TEXT;
uint16_t COLOR_ACCENT;
uint16_t COLOR_SUN;
uint16_t COLOR_MOON;
uint16_t COLOR_CLOUD;
uint16_t COLOR_CLOUD_DARK;
uint16_t COLOR_RAIN;
uint16_t COLOR_BOLT;
uint16_t COLOR_SNOW;
uint16_t COLOR_MIST;

enum WeatherIconType {
    WI_CLEAR_DAY,
    WI_CLEAR_NIGHT,
    WI_PARTLY_CLOUDY,
    WI_CLOUDY,
    WI_RAIN,
    WI_THUNDERSTORM,
    WI_SNOW,
    WI_MIST
};

static WeatherIconType iconTypeFromCode(const String &code) {
    if (code.length() < 3) return WI_CLOUDY;
    String prefix = code.substring(0, 2);
    bool night = code.endsWith("n");
    if (prefix == "01") return night ? WI_CLEAR_NIGHT : WI_CLEAR_DAY;
    if (prefix == "02" || prefix == "03") return WI_PARTLY_CLOUDY;
    if (prefix == "04") return WI_CLOUDY;
    if (prefix == "09" || prefix == "10") return WI_RAIN;
    if (prefix == "11") return WI_THUNDERSTORM;
    if (prefix == "13") return WI_SNOW;
    if (prefix == "50") return WI_MIST;
    return WI_CLOUDY;
}

static void drawSun(Arduino_GFX *gfx, int cx, int cy, int r, uint16_t color) {
    gfx->fillCircle(cx, cy, r * 0.5, color);
    for (int i = 0; i < 8; i++) {
        float a = i * (float)M_PI / 4.0f;
        int x1 = cx + cosf(a) * r * 0.68f;
        int y1 = cy + sinf(a) * r * 0.68f;
        int x2 = cx + cosf(a) * r;
        int y2 = cy + sinf(a) * r;
        gfx->drawLine(x1, y1, x2, y2, color);
    }
}

static void drawMoon(Arduino_GFX *gfx, int cx, int cy, int r, uint16_t color, uint16_t bg) {
    gfx->fillCircle(cx, cy, r * 0.6, color);
    gfx->fillCircle(cx + r * 0.3, cy - r * 0.15, r * 0.55, bg);
}

static void drawCloudShape(Arduino_GFX *gfx, int cx, int cy, int r, uint16_t color) {
    gfx->fillCircle(cx - r / 2, cy + r / 6, r / 2, color);
    gfx->fillCircle(cx, cy - r / 4, r * 0.6, color);
    gfx->fillCircle(cx + r / 2, cy + r / 6, r / 2, color);
    gfx->fillRoundRect(cx - r / 2, cy, r, r / 2, r / 6, color);
}

static void drawPartlyCloudy(Arduino_GFX *gfx, int cx, int cy, int r, uint16_t sunColor, uint16_t cloudColor) {
    drawSun(gfx, cx - r / 3, cy - r / 3, r * 0.5, sunColor);
    drawCloudShape(gfx, cx + r / 6, cy + r / 6, r * 0.85, cloudColor);
}

static void drawRain(Arduino_GFX *gfx, int cx, int cy, int r, uint16_t cloudColor, uint16_t dropColor) {
    drawCloudShape(gfx, cx, cy - r / 6, r, cloudColor);
    for (int i = -1; i <= 1; i++) {
        int x = cx + i * r / 2;
        int y = cy + r / 3;
        gfx->drawLine(x, y, x - 3, y + 10, dropColor);
    }
}

static void drawThunder(Arduino_GFX *gfx, int cx, int cy, int r, uint16_t cloudColor, uint16_t boltColor) {
    drawCloudShape(gfx, cx, cy - r / 6, r, cloudColor);
    int x0 = cx + 2, y0 = cy + r / 3;
    gfx->fillTriangle(x0, y0, x0 - 10, y0 + 14, x0 + 2, y0 + 8, boltColor);
    gfx->fillTriangle(x0 + 2, y0 + 8, x0 - 6, y0 + 22, x0 + 10, y0 + 6, boltColor);
}

static void drawSnow(Arduino_GFX *gfx, int cx, int cy, int r, uint16_t cloudColor, uint16_t flakeColor) {
    drawCloudShape(gfx, cx, cy - r / 6, r, cloudColor);
    for (int i = -1; i <= 1; i++) {
        int x = cx + i * r / 2;
        int y = cy + r / 2;
        gfx->drawLine(x - 4, y, x + 4, y, flakeColor);
        gfx->drawLine(x, y - 4, x, y + 4, flakeColor);
        gfx->drawLine(x - 3, y - 3, x + 3, y + 3, flakeColor);
        gfx->drawLine(x - 3, y + 3, x + 3, y - 3, flakeColor);
    }
}

static void drawMist(Arduino_GFX *gfx, int cx, int cy, int r, uint16_t color) {
    for (int i = 0; i < 4; i++) {
        int y = cy - r / 2 + i * (r / 2);
        gfx->drawFastHLine(cx - r, y, r * 2, color);
        gfx->drawFastHLine(cx - r + 6, y + 4, r * 2 - 12, color);
    }
}

void drawWeatherIcon(Arduino_GFX *gfx, int x, int y, int size, const String &iconCode) {
    int cx = x + size / 2;
    int cy = y + size / 2;
    int r = size / 2;

    switch (iconTypeFromCode(iconCode)) {
        case WI_CLEAR_DAY:
            drawSun(gfx, cx, cy, r, COLOR_SUN);
            break;
        case WI_CLEAR_NIGHT:
            drawMoon(gfx, cx, cy, r, COLOR_MOON, COLOR_BG);
            break;
        case WI_PARTLY_CLOUDY:
            drawPartlyCloudy(gfx, cx, cy, r, COLOR_SUN, COLOR_CLOUD);
            break;
        case WI_CLOUDY:
            drawCloudShape(gfx, cx, cy, r, COLOR_CLOUD_DARK);
            break;
        case WI_RAIN:
            drawRain(gfx, cx, cy, r, COLOR_CLOUD_DARK, COLOR_RAIN);
            break;
        case WI_THUNDERSTORM:
            drawThunder(gfx, cx, cy, r, COLOR_CLOUD_DARK, COLOR_BOLT);
            break;
        case WI_SNOW:
            drawSnow(gfx, cx, cy, r, COLOR_CLOUD, COLOR_SNOW);
            break;
        case WI_MIST:
            drawMist(gfx, cx, cy, r, COLOR_MIST);
            break;
    }
}
