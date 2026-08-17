# ESP32-S3 Weather Display

A small weather station firmware for the [LILYGO T-Display-S3](https://github.com/Xinyuan-LilyGO/T-Display-S3) (ESP32-S3, 1.9" ST7789 LCD, 170x320). It fetches current conditions and a 3-day forecast from [OpenWeatherMap](https://openweathermap.org/) and cycles between two screens.

## Features

- **Current weather screen** — city name, temperature, hand-drawn weather icon, description, "feels like" temperature, humidity, and wind speed.
- **3-day forecast screen** — daily min/max temperature and icon for the next 3 days.
- Screens auto-rotate on a timer; weather data refreshes periodically in the background.
- Weather icons (sun, moon, clouds, rain, thunderstorm, snow, mist) are drawn directly with `Arduino_GFX` primitives — no icon font or image assets needed.

## Hardware

- Board: LILYGO T-Display-S3 (ESP32-S3-R8, 8MB PSRAM, built-in ST7789 LCD)
- Pin mapping is in [`src/pin_config.h`](src/pin_config.h) (matches the board's official factory example)

## Requirements

- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- A free [OpenWeatherMap API key](https://openweathermap.org/api)

## Setup

1. Clone this repository.
2. Open `src/config.h` and fill in your own values:

   ```cpp
   #define WIFI_SSID       "YOUR_WIFI_SSID"
   #define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"
   #define OWM_API_KEY     "YOUR_OPENWEATHERMAP_API_KEY"
   #define OWM_CITY        "Frankfurt"
   #define OWM_COUNTRY     "DE"
   #define OWM_LANG        "en"
   #define OWM_UNITS       "metric"
   ```

   `OWM_CITY`/`OWM_COUNTRY` accept any city recognized by the OpenWeatherMap API. `OWM_UNITS` can be `metric`, `imperial`, or `standard`.

3. Build and upload with PlatformIO:

   ```
   pio run -t upload
   ```

4. Optionally monitor serial output:

   ```
   pio device monitor
   ```

## Configuration

Screen timing and refresh rate are also in `src/config.h`:

```cpp
#define CURRENT_SCREEN_MS      30000UL   // time on the current weather screen
#define FORECAST_SCREEN_MS     10000UL   // time on the forecast screen
#define DATA_REFRESH_MS        (10UL * 60UL * 1000UL)  // how often to re-fetch the API
```

## Project structure

```
src/
  main.cpp          # app logic, screens, WiFi + OpenWeatherMap fetching
  weather_icons.h/.cpp  # hand-drawn weather icons
  pin_config.h       # T-Display-S3 pin mapping
  config.h           # WiFi / API / timing configuration (edit this)
```

## License

No license specified — all rights reserved by default. Add a license file if you want to allow reuse.
