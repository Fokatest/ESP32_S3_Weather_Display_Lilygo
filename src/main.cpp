#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Arduino_GFX_Library.h>

#include "pin_config.h"
#include "config.h"
#include "weather_icons.h"

// ---------------- Display ----------------
Arduino_DataBus *bus = new Arduino_ESP32PAR8Q(
    PIN_LCD_DC, PIN_LCD_CS, PIN_LCD_WR, PIN_LCD_RD,
    PIN_LCD_D0, PIN_LCD_D1, PIN_LCD_D2, PIN_LCD_D3,
    PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);

Arduino_GFX *gfx = new Arduino_ST7789(
    bus, PIN_LCD_RES, 1 /* rotation: 1 = landscape 320x170 */, true,
    LCD_WIDTH, LCD_HEIGHT, 35, 0, 35, 0);

// ---------------- Data model ----------------
struct CurrentWeather {
    bool valid = false;
    float tempC = 0;
    float feelsLike = 0;
    int humidity = 0;
    float windSpeed = 0;
    String description;
    String icon = "01d";
    String cityName;
};

struct DayForecast {
    bool valid = false;
    int y = 0, mo = 0, d = 0, dow = 0;
    float tempMin = 0, tempMax = 0;
    int bestHourDist = 99;
    String icon = "01d";
    String description;
};

CurrentWeather currentWeather;
DayForecast forecastDays[3];

enum ScreenState { SCREEN_CURRENT, SCREEN_FORECAST };
ScreenState screenState = SCREEN_CURRENT;
unsigned long screenSwitchAt = 0;
unsigned long lastDataFetch = 0;
bool haveData = false;

// ---------------- Date helpers ----------------
// Howard Hinnant's algorithm (civil_from_days) - converts days since 1970-01-01 into (year, month, day)
static void civilFromDays(long z, int &y, int &m, int &d) {
    z += 719468;
    long era = (z >= 0 ? z : z - 146096) / 146097;
    unsigned long doe = (unsigned long)(z - era * 146097);
    unsigned long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    long yr = (long)yoe + era * 400;
    unsigned long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    unsigned long mp = (5 * doy + 2) / 153;
    d = doy - (153 * mp + 2) / 5 + 1;
    m = mp + (mp < 10 ? 3 : -9);
    y = yr + (m <= 2 ? 1 : 0);
}

static int weekdayFromDays(long z) {
    // 1970-01-01 (z=0) was a Thursday
    long w = (z + 4) % 7;
    if (w < 0) w += 7;
    return (int)w; // 0=Sunday ... 6=Saturday
}

static const char *weekdayAbbrev(int dow) {
    static const char *names[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    return names[dow % 7];
}

static const char *monthAbbrev(int mo) {
    static const char *names[] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    return names[mo];
}

// ---------------- WiFi ----------------
static void drawStatusMessage(const String &msg) {
    gfx->fillScreen(COLOR_BG);
    gfx->setTextColor(COLOR_TEXT);
    gfx->setTextSize(2);
    gfx->setCursor(10, 70);
    gfx->print(msg);
}

static void connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    drawStatusMessage("Connecting WiFi...");
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
        delay(300);
    }
}

// ---------------- OpenWeatherMap ----------------
static bool fetchCurrentWeather(CurrentWeather &out) {
    if (WiFi.status() != WL_CONNECTED) return false;

    WiFiClientSecure client;
    client.setInsecure(); // no certificate check - fine for public weather data

    String url = "https://api.openweathermap.org/data/2.5/weather?q=" +
                 String(OWM_CITY) + "," + String(OWM_COUNTRY) +
                 "&appid=" + String(OWM_API_KEY) +
                 "&units=" + String(OWM_UNITS) + "&lang=" + String(OWM_LANG);

    HTTPClient http;
    if (!http.begin(client, url)) return false;
    http.useHTTP10(true);
    int code = http.GET();
    if (code != 200) {
        http.end();
        return false;
    }

    JsonDocument filter;
    filter["main"]["temp"] = true;
    filter["main"]["feels_like"] = true;
    filter["main"]["humidity"] = true;
    filter["wind"]["speed"] = true;
    filter["weather"][0]["description"] = true;
    filter["weather"][0]["icon"] = true;
    filter["name"] = true;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
    http.end();
    if (err) return false;

    out.tempC = doc["main"]["temp"] | 0.0f;
    out.feelsLike = doc["main"]["feels_like"] | 0.0f;
    out.humidity = doc["main"]["humidity"] | 0;
    out.windSpeed = doc["wind"]["speed"] | 0.0f;
    out.description = doc["weather"][0]["description"] | "";
    out.icon = doc["weather"][0]["icon"] | "01d";
    out.cityName = doc["name"] | String(OWM_CITY);
    out.valid = true;
    return true;
}

static bool fetchForecast(DayForecast days[3]) {
    if (WiFi.status() != WL_CONNECTED) return false;

    WiFiClientSecure client;
    client.setInsecure();

    String url = "https://api.openweathermap.org/data/2.5/forecast?q=" +
                 String(OWM_CITY) + "," + String(OWM_COUNTRY) +
                 "&appid=" + String(OWM_API_KEY) +
                 "&units=" + String(OWM_UNITS) + "&lang=" + String(OWM_LANG);

    HTTPClient http;
    if (!http.begin(client, url)) return false;
    http.useHTTP10(true);
    int code = http.GET();
    if (code != 200) {
        http.end();
        return false;
    }

    JsonDocument filter;
    filter["city"]["timezone"] = true;
    filter["list"][0]["dt"] = true;
    filter["list"][0]["main"]["temp"] = true;
    filter["list"][0]["weather"][0]["description"] = true;
    filter["list"][0]["weather"][0]["icon"] = true;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
    http.end();
    if (err) return false;

    long tz = doc["city"]["timezone"] | 0L;

    for (int i = 0; i < 3; i++) {
        days[i] = DayForecast();
    }

    long currentDateKey = -1;
    int dayCount = 0;

    for (JsonObject item : doc["list"].as<JsonArray>()) {
        long dt = item["dt"] | 0L;
        long localDt = dt + tz;
        long dayNum = localDt >= 0 ? localDt / 86400 : (localDt - 86399) / 86400;
        int y, mo, d;
        civilFromDays(dayNum, y, mo, d);
        long dateKey = (long)y * 10000 + mo * 100 + d;

        if (dateKey != currentDateKey) {
            currentDateKey = dateKey;
            dayCount++;
        }

        int idx = dayCount - 2; // day 0 = today (partial, ignored) -> idx -1
        if (idx < 0 || idx >= 3) continue;

        float temp = item["main"]["temp"] | 0.0f;
        String icon = item["weather"][0]["icon"] | "01d";
        String desc = item["weather"][0]["description"] | "";

        DayForecast &df = days[idx];
        if (!df.valid) {
            df.valid = true;
            df.y = y; df.mo = mo; df.d = d;
            df.dow = weekdayFromDays(dayNum);
            df.tempMin = temp;
            df.tempMax = temp;
        } else {
            if (temp < df.tempMin) df.tempMin = temp;
            if (temp > df.tempMax) df.tempMax = temp;
        }

        int secOfDay = (int)(((localDt % 86400) + 86400) % 86400);
        int hour = secOfDay / 3600;
        int distTo12 = abs(hour - 12);
        if (distTo12 < df.bestHourDist) {
            df.bestHourDist = distTo12;
            df.icon = icon;
            df.description = desc;
        }

        if (dayCount > 4) break;
    }

    return days[0].valid;
}

static void refreshData() {
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.reconnect();
    }
    bool okCurrent = fetchCurrentWeather(currentWeather);
    bool okForecast = fetchForecast(forecastDays);
    haveData = okCurrent && okForecast;
}

// ---------------- Draw screens ----------------
static void drawCurrentScreen(const CurrentWeather &cw) {
    gfx->fillScreen(COLOR_BG);

    if (!cw.valid) {
        drawStatusMessage("No weather data");
        return;
    }

    gfx->setTextColor(COLOR_TEXT);
    gfx->setTextSize(2);
    gfx->setCursor(8, 6);
    gfx->print(cw.cityName);

    drawWeatherIcon(gfx, 10, 40, 100, cw.icon);

    gfx->setTextSize(5);
    gfx->setTextColor(COLOR_ACCENT);
    gfx->setCursor(130, 40);
    char tempBuf[8];
    snprintf(tempBuf, sizeof(tempBuf), "%.0f", cw.tempC);
    gfx->print(tempBuf);

    int dx = gfx->getCursorX() + 5;
    gfx->drawCircle(dx, 44, 5, COLOR_ACCENT);
    gfx->setCursor(dx + 14, 40);
    gfx->print("C");

    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_TEXT);
    gfx->setCursor(130, 100);
    gfx->print(cw.description);

    gfx->setTextSize(2);
    char row1[40];
    snprintf(row1, sizeof(row1), "Feels %.0fC  Hum %d%%", cw.feelsLike, cw.humidity);
    gfx->setCursor(10, 132);
    gfx->print(row1);

    char row2[24];
    snprintf(row2, sizeof(row2), "Wind %.1f m/s", cw.windSpeed);
    gfx->setCursor(10, 150);
    gfx->print(row2);
}

static void drawForecastScreen(DayForecast days[3]) {
    gfx->fillScreen(COLOR_BG);
    gfx->setTextColor(COLOR_TEXT);
    gfx->setTextSize(2);
    gfx->setCursor(8, 4);
    gfx->print("3-Day Forecast");

    int colW = 320 / 3;
    for (int i = 0; i < 3; i++) {
        int x0 = i * colW;

        if (i > 0) gfx->drawFastVLine(x0, 30, 140, COLOR_CLOUD_DARK);

        if (!days[i].valid) continue;

        gfx->setTextSize(1);
        gfx->setTextColor(COLOR_TEXT);
        gfx->setCursor(x0 + 8, 34);
        char label[24];
        snprintf(label, sizeof(label), "%s, %d %s", weekdayAbbrev(days[i].dow), days[i].d, monthAbbrev(days[i].mo));
        gfx->print(label);

        drawWeatherIcon(gfx, x0 + colW / 2 - 28, 50, 56, days[i].icon);

        gfx->setTextSize(2);
        gfx->setTextColor(COLOR_ACCENT);
        gfx->setCursor(x0 + 12, 122);
        char tbuf[16];
        snprintf(tbuf, sizeof(tbuf), "%.0f/%.0f", days[i].tempMax, days[i].tempMin);
        gfx->print(tbuf);
    }
}

// ---------------- Arduino lifecycle ----------------
void setup() {
    Serial.begin(115200);

    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);

    pinMode(PIN_LCD_BL, OUTPUT);
    digitalWrite(PIN_LCD_BL, HIGH);

    gfx->begin();

    COLOR_BG = gfx->color565(10, 15, 25);
    COLOR_TEXT = gfx->color565(235, 235, 240);
    COLOR_ACCENT = gfx->color565(90, 170, 255);
    COLOR_SUN = gfx->color565(255, 190, 60);
    COLOR_MOON = gfx->color565(200, 205, 215);
    COLOR_CLOUD = gfx->color565(210, 215, 225);
    COLOR_CLOUD_DARK = gfx->color565(140, 150, 165);
    COLOR_RAIN = gfx->color565(80, 150, 255);
    COLOR_BOLT = gfx->color565(255, 220, 60);
    COLOR_SNOW = gfx->color565(255, 255, 255);
    COLOR_MIST = gfx->color565(170, 180, 190);

    gfx->fillScreen(COLOR_BG);

    connectWiFi();

    if (WiFi.status() == WL_CONNECTED) {
        drawStatusMessage("Fetching weather...");
        refreshData();
    }

    screenState = SCREEN_CURRENT;
    drawCurrentScreen(currentWeather);
    screenSwitchAt = millis() + CURRENT_SCREEN_MS;
    lastDataFetch = millis();
}

void loop() {
    unsigned long now = millis();

    if (now - lastDataFetch >= DATA_REFRESH_MS) {
        refreshData();
        lastDataFetch = now;
        if (screenState == SCREEN_CURRENT) {
            drawCurrentScreen(currentWeather);
        } else {
            drawForecastScreen(forecastDays);
        }
    }

    if (now >= screenSwitchAt) {
        if (screenState == SCREEN_CURRENT) {
            screenState = SCREEN_FORECAST;
            drawForecastScreen(forecastDays);
            screenSwitchAt = now + FORECAST_SCREEN_MS;
        } else {
            screenState = SCREEN_CURRENT;
            drawCurrentScreen(currentWeather);
            screenSwitchAt = now + CURRENT_SCREEN_MS;
        }
    }

    delay(200);
}
