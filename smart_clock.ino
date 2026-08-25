#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <time.h>
#include <MD_Parola.h>
#include <SPI.h>

#include "led_indicator.h"
#include "littleFS_config.h"
#include "dns_config.h"
#include "wifi_manager_config.h"
#include "blynk_config.h"
#include "Font_Data.h"
#include "index_page.h"
#include "ota_page.h"

String deviceLocation = "";
String version = "1.0.11";

// GEOLOCATION & TIMEZONE ================
// Timezone in seconds: Auto-detected from geolocation during startup
// Fallback: 0 = UTC (will be overridden by geolocation)
int32_t TIMEZONE_SECONDS = 0;
// Geolocation API endpoint
const char *GEOLOCATION_API = "http://ip-api.com/json/?fields=country,city,lat,lon,timezone,offset";
bool geoSync = false;
unsigned long lastGeoSync = 0;
const unsigned long RETRY_GEOSYNC_MS = 10000; // 10 seconds timeout before retry
// Geolocation data
struct
{
    char country[32];
    char city[32];
    float latitude;
    float longitude;
    char timezone[40];
    int32_t utcOffset; // UTC offset in seconds
} location;
void initGeoLocation()
{
    geoSync = getKeyValue("geo_sync", "0").toInt() == 1;
    TIMEZONE_SECONDS = getKeyValue("TIMEZONE_SECONDS", "25200").toInt();
    Serial.printf("Last Geolocation Sync: %s\n", geoSync == 1 ? "TRUE" : "FALSE");
    Serial.printf("Last timezone offset (seconds): %d\n", TIMEZONE_SECONDS);
}
void fetchGeolocation()
{
    if (geoSync)
        return;

    Serial.println("Fetching geolocation and timezone...");

    WiFiClient client;
    HTTPClient http;
    http.begin(client, GEOLOCATION_API);
    http.setTimeout(5000);

    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK)
    {
        Serial.println("Geolocation API request failed");
        http.end();
        lastGeoSync = millis();
        return;
    }

    String payload = http.getString();
    http.end();

    Serial.print("Geolocation response: ");
    Serial.println(payload);

    // Parse JSON response
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        lastGeoSync = millis();
        return;
    }

    // Extract data
    strlcpy(location.country, doc["country"] | "", sizeof(location.country));
    strlcpy(location.city, doc["city"] | "", sizeof(location.city));
    location.latitude = doc["lat"] | 0.0f;
    location.longitude = doc["lon"] | 0.0f;
    strlcpy(location.timezone, doc["timezone"] | "", sizeof(location.timezone));
    location.utcOffset = doc["offset"] | 0;

    // Update global timezone offset
    TIMEZONE_SECONDS = location.utcOffset;

    Serial.printf("Location: %s, %s\n", location.city, location.country);
    Serial.printf("Coordinates: %.4f, %.4f\n", location.latitude, location.longitude);
    Serial.printf("Timezone: %s (UTC offset: %d seconds)\n", location.timezone, location.utcOffset);

    setKeyValue("TIMEZONE_SECONDS", String(TIMEZONE_SECONDS).c_str());
    setKeyValue("geo_sync", "1");
    geoSync = true;
}
// GEOLOCATION & TIMEZONE ================

// NTP Synchronization ================
const int DST = 0;
// NTP servers for time synchronization
const char *NTP_SERVERS[] = {"pool.ntp.org", "time.nist.gov"};
bool timedSync = false;
unsigned long lastTimedSync = 0;
const unsigned long RETRY_TIMEDSYNC_MS = 10000; // 10 seconds timeout before retry
void syncTimeFromNTP()
{
    if (timedSync)
        return;

    Serial.println("Synchronizing time from NTP...");
    Serial.print("Using timezone offset (seconds): ");
    Serial.println(TIMEZONE_SECONDS);

    configTime(TIMEZONE_SECONDS, DST, NTP_SERVERS[0], NTP_SERVERS[1]);

    // Wait for time to be set (max 10 seconds)
    uint32_t startTime = millis();
    time_t now = time(nullptr);

    while (now < 24 * 3600 && (millis() - startTime) < 10000)
    {
        delay(100);
        now = time(nullptr);
    }

    if (now > 24 * 3600)
    {
        Serial.println("Time synchronized successfully");
        // Print the synchronized time
        time_t syncTime = time(nullptr);
        struct tm *p_tm = localtime(&syncTime);
        Serial.print("Fetched UTC time: ");
        Serial.print(p_tm->tm_year + 1900);
        Serial.print("-");
        Serial.print(p_tm->tm_mon + 1);
        Serial.print("-");
        Serial.print(p_tm->tm_mday);
        Serial.print(" ");
        Serial.print(p_tm->tm_hour);
        Serial.print(":");
        Serial.print(p_tm->tm_min);
        Serial.print(":");
        Serial.println(p_tm->tm_sec);
        timedSync = true;
    }
    else
    {
        Serial.println("Failed to synchronize time from NTP.");
        timedSync = false;
        lastTimedSync = millis();
    }
}
// NTP Synchronization ================

// Time configuration ================
uint16_t h, m, s;
uint16_t year, month, day;
bool use12HourFormat = false;
unsigned long lastTimeUpdate = 0;
static bool flasher = false;
void initTime()
{
    use12HourFormat = getKeyValue("use12HourFormat", "0").toInt() == 1;
}
String getTimeString()
{
    time_t now = time(nullptr);
    struct tm *p_tm = localtime(&now);

    if (use12HourFormat)
    {
        h = p_tm->tm_hour % 12;
        h = h == 0 ? 12 : h;
    }
    else
    {
        h = p_tm->tm_hour;
    }

    m = p_tm->tm_min;
    s = p_tm->tm_sec;

    char text[9];
    snprintf(text, sizeof(text), "%02d%c%02d%c%02d", h, (flasher ? ':' : ' '), m, (flasher ? ':' : ' '), s);
    flasher = !flasher;

    return text;
}
String getDateString()
{
    time_t now = time(nullptr);
    struct tm *p_tm = localtime(&now);

    year = p_tm->tm_year + 1900;
    month = p_tm->tm_mon + 1;
    day = p_tm->tm_mday;

    char text[11];
    snprintf(text, sizeof(text), "%04d %02d %02d", year, month, day);

    return text;
}
// Time configuration ================

// Display configuration ================
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 5
#define CLK_PIN D5  // SCK
#define DATA_PIN D7 // MOSI
#define CS_PIN D8   // SS / CS
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

#define SPEED_TIME 75
#define PAUSE_TIME 0
#define MAX_MESG 20
int brightness = 1;
int currState = 1; // 1 = time | 2 = date
enum DisplayState
{
    SHOW_CLOCK,
    SHOW_DATE
};
DisplayState currDisplayState = SHOW_CLOCK;
unsigned long stateMillis = 0;
unsigned long SHOW_CLOCK_DELAY_MS = 60000; // 60 seconds
unsigned long SHOW_DATE_DELAY_MS = 30000;  // 30 seconds
String msg;

void initDisplay()
{
    brightness = getKeyValue("brightness", "1").toInt();
    SHOW_CLOCK_DELAY_MS = getKeyValue("SHOW_CLOCK_DELAY_MS", "60000").toInt();
    SHOW_DATE_DELAY_MS = getKeyValue("SHOW_DATE_DELAY_MS", "30000").toInt();

    msg = getDateString();

    P.begin();
    P.displayClear();
}
void scrollingDisplay(const char *text, uint16_t speed, uint16_t pause)
{
    P.displayClear();
    P.setIntensity(brightness);
    P.setFont(numeric7Seg);
    P.displayText(text, PA_CENTER, speed, pause, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    P.displayAnimate();
}
void staticDisplay(const char *text)
{
    P.setIntensity(brightness);
    P.setFont(numeric7Se);
    P.displayText(text, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);
    P.displayAnimate();
}
void updateDisplay()
{
    // displayAnimate handles non-blocking frame updates automatically
    if (currDisplayState == SHOW_DATE)
    {
        if (P.displayAnimate())
        {
            P.displayReset();
        }
    }
    // check update display every second
    if (millis() - lastTimeUpdate >= 1000)
    {
        lastTimeUpdate = millis();
        if (geoSync && timedSync)
        {
            // Update clock every second, for animated the blink
            if (currDisplayState == SHOW_CLOCK)
            {
                msg = getTimeString();
                Serial.printf("Display Clock: %s\n", msg);
                staticDisplay(msg.c_str());
            }
            // when the SHOW_CLOCK reach the target delay
            if (currDisplayState == SHOW_CLOCK && millis() - stateMillis >= SHOW_CLOCK_DELAY_MS)
            {
                currDisplayState = SHOW_DATE;
                stateMillis = millis();

                msg = getDateString();
                Serial.printf("Display Date: %s\n", msg);
                scrollingDisplay(msg.c_str(), 50, SHOW_DATE_DELAY_MS);
            }
            // when the SHOW_DATE reach the target delay
            if (currDisplayState == SHOW_DATE && millis() - stateMillis >= SHOW_DATE_DELAY_MS)
            {
                currDisplayState = SHOW_CLOCK;
                stateMillis = millis();
            }
        }
    }
}
// Display configuration ================

// WEBSERVER ============================
ESP8266HTTPUpdateServer httpUpdater;
ESP8266WebServer server(80);
void handleRoot()
{
    String html = INDEX_PAGE;
    html.replace("{version}", String(version));
    html.replace("{device_location}", String(deviceLocation));
    html.replace("{ip_address}", WiFi.localIP().toString());
    html.replace("{dns_name}", "http://" + localDNS + ".local");
    html.replace("{brightness}", String(brightness) + "/15");
    html.replace("{brightness_input}", String(brightness));
    html.replace("{time_format}", String(use12HourFormat == 1 ? "12 hour (AM/PM)" : "24 hour"));
    server.send_P(200, "text/html", html.c_str());
}
void handleData()
{
    String json = "{";
    json += "\"device_location\":\"" + String(deviceLocation) + "\"";
    json += ", \"version\":\"" + String(version) + "\"";
    json += ", \"brightness\":\"" + String(brightness) + "\"";
    json += ", \"time_format\":\"" + String(use12HourFormat ? 1 : 0) + "\"";
    json += "}";

    server.sendHeader("Cache-Control", "no-cache");
    server.send(200, "application/json", json);
}
void handleSetDeviceLocation()
{
    if (server.hasArg("device_location"))
    {
        deviceLocation = server.arg("device_location");
        setKeyValue("device_location", deviceLocation.c_str());
    }
    server.sendHeader("Location", "/");
    server.send(303);
}
void handleSetBrightness()
{
    if (server.hasArg("brightness"))
    {
        brightness = server.arg("brightness").toInt();
        setKeyValue("brightness", String(brightness).c_str());
    }
    server.sendHeader("Location", "/");
    server.send(303);
}
void handleSetTimeFormat()
{
    if (server.hasArg("time_format"))
    {
        use12HourFormat = server.arg("time_format").toInt() == 1;
        setKeyValue("use12HourFormat", String(use12HourFormat ? "1" : "0").c_str());
    }
    server.sendHeader("Location", "/");
    server.send(303);
}
void handleWebResetWiFi()
{
    server.sendHeader("Location", "/");
    server.send(303);
    wifiManager.resetSettings();
    delay(3000);
    ESP.restart(); // Reset and try again
}
void initWebserver()
{
    // Define what happens when you visit the root IP/URL
    server.on("/", handleRoot);
    server.on("/data", handleData);
    server.on("/set_device_location", handleSetDeviceLocation);
    server.on("/set_brightness", handleSetBrightness);
    server.on("/set_time_format", handleSetTimeFormat);
    server.on("/reset_wifi", handleWebResetWiFi);

    // Define what happens when you visit the OTA update page
    server.on("/server-ota", []()
              { 
    String html = OTA_PAGE;
    html.replace("{version}", String(version));
    server.send(200, "text/html", html); });
    //   Setup OTA Update Server
    httpUpdater.setup(&server);

    // Start the web server
    server.begin();

    Serial.println("HTTP server started");
}
void runWebServer()
{
    server.handleClient();
}
// WEBSERVER ============================

void setup()
{
    Serial.begin(115200);

    initLittleFS();
    deviceLocation = getKeyValue("device_location", "");
    initLedIndicator();
    initGeoLocation();
    initTime();
    // setupEye();

    initConnection();
    // Setup mDNS for local network access
    initDNS();
}

void loop()
{
    wifiManager.process();
    updateLedIndicator();
    runDNS();
    updateDisplay();
    checkConnection([](bool connected)
                    {
        if (connected) {
            // This section only running once, after connection establish !
            Serial.println("This only running once !");
            // initBlynk();
            initWebserver();
            fetchGeolocation();
            syncTimeFromNTP();
            initDisplay();
            currLedState = CONNECTED;
        } });
    // This section will running after the connection established !
    if (isConnected)
    {
        // runBlynk();
        runWebServer();
        if (!geoSync && millis() - lastGeoSync >= RETRY_GEOSYNC_MS)
            fetchGeolocation();
        if (geoSync && !timedSync && millis() - lastTimedSync >= RETRY_TIMEDSYNC_MS)
            syncTimeFromNTP();
    }
}
