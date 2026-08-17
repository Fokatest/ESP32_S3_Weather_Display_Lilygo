#pragma once

// ---- WiFi ----
#define WIFI_SSID       "YOUR_WIFI_SSID"
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"

// ---- OpenWeatherMap ----
// Get a free API key at https://openweathermap.org/api
#define OWM_API_KEY     "YOUR_OPENWEATHERMAP_API_KEY"
#define OWM_CITY        "Frankfurt"
#define OWM_COUNTRY     "DE"
#define OWM_LANG        "en"
#define OWM_UNITS       "metric"

// ---- Screen timing (milliseconds) ----
#define CURRENT_SCREEN_MS      30000UL   // 30s current weather screen
#define FORECAST_SCREEN_MS     10000UL   // 10s 3-day forecast screen
#define DATA_REFRESH_MS        (10UL * 60UL * 1000UL)  // re-fetch API every 10 minutes
