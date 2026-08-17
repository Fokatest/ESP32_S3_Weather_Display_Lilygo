#pragma once
#include <Arduino_GFX_Library.h>

// Shared colors, initialized in main.cpp after the display is set up
extern uint16_t COLOR_BG;
extern uint16_t COLOR_TEXT;
extern uint16_t COLOR_ACCENT;
extern uint16_t COLOR_SUN;
extern uint16_t COLOR_MOON;
extern uint16_t COLOR_CLOUD;
extern uint16_t COLOR_CLOUD_DARK;
extern uint16_t COLOR_RAIN;
extern uint16_t COLOR_BOLT;
extern uint16_t COLOR_SNOW;
extern uint16_t COLOR_MIST;

// Draws the weather icon matching the OpenWeatherMap icon code (e.g. "01d", "10n")
// inside a `size` x `size` square, top-left corner at (x, y).
void drawWeatherIcon(Arduino_GFX *gfx, int x, int y, int size, const String &iconCode);
