#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Adafruit_AHT10.h>
#include <Wire.h>
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

#define SDA_PIN 4 // D2
#define SCL_PIN 5 // D1

enum DisplayState
{
    SHOW_CONNECTION_SETUP,
    SHOW_CONNECTION_FAILED,
    SHOW_TIMEZONE_FAILED,
    SHOW_NTP_FAILED,
    SHOW_CLOCK,
    SHOW_DATE,
    SHOW_TEMP_HUM
};
DisplayState displayState = SHOW_CLOCK;

String deviceLocation = "";
String version = "1.1.0";

// TEMP & HUM ===========================
Adafruit_AHT10 aht;
float temperature = 0.0;
float humidity = 0.0;
float offsetTemp = -2;
float offsetHum = -12;
unsigned long lastSensorReadTime = 0;
const unsigned long SENSOR_READ_INTERVAL = 5000; // Read sensors every 5 seconds
void initAHTSensor()
{
    if (!aht.begin(&Wire, 0x38))
    {
        Serial.println("Could not find AHT10 sensor!");
        // We don't freeze the program here so the counter can still work without the sensor
    }

    // Initial read to populate screen data immediately
    offsetTemp = getKeyValue("offsetTemp", String(offsetTemp).c_str()).toFloat();
    offsetHum = getKeyValue("offsetHum", String(offsetHum).c_str()).toFloat();
    readAHTSensor();
}
void readAHTSensor()
{
    sensors_event_t humidityEvent, tempEvent;
    if (aht.getEvent(&humidityEvent, &tempEvent))
    {
        temperature = tempEvent.temperature + offsetTemp;
        humidity = humidityEvent.relative_humidity + offsetHum;

        // Serial.print(F("Temperature: "));
        // Serial.print(temperature);
        // Serial.print(F(" °C Humidity: "));
        // Serial.print(humidity);
        // Serial.println(F(" %"));
    }
}
void updateAHTSensor()
{
    if (millis() - lastSensorReadTime >= SENSOR_READ_INTERVAL)
    {
        readAHTSensor();
        lastSensorReadTime = millis();
    }
}
String getTempHumString()
{
    String text = String(temperature, 1) + "\xB0" + "C " + String(humidity, 1) + "%";
    return text;
}
// TEMP & HUM ===========================

// LDR Sensor =========================
const int LDR_PIN = A0;
unsigned long lastLdrRead = 0;
const long LDR_INTERVAL = 1000;
void updateLDRSensor()
{
    if (millis() - lastLdrRead >= LDR_INTERVAL)
    {
        lastLdrRead = millis();

        int ldrValue = analogRead(LDR_PIN);
        Serial.printf("Analog (Light Intensity): %d\n", ldrValue);
    }
}
// LDR Sensor =========================

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
    Serial.printf("Last Geolocation Sync: %s | Timezone offset (seconds): %d\n", geoSync == 1 ? "TRUE" : "FALSE", TIMEZONE_SECONDS);
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
        geoSync = false;
        displayState = SHOW_TIMEZONE_FAILED;
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
        geoSync = false;
        displayState = SHOW_TIMEZONE_FAILED;
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
const char *NTP_SERVERS[] = {"pool.ntp.org", "time.nist.gov", "asia.pool.ntp.org"};
bool ntpSync = false;
unsigned long lastNTPSync = 0;
const unsigned long RETRY_NTPSYNC_MS = 10000;    // retry every 10 seconds when sync FAILED
const unsigned long INTERVAL_NTPSYNC_MS = 60000 * 60; // every 1 hours sync to NTP server
void syncTimeFromNTP()
{
    Serial.println("Synchronizing time from NTP...");
    Serial.print("Using timezone offset (seconds): ");
    Serial.println(TIMEZONE_SECONDS);

    lastNTPSync = millis();

    configTime(TIMEZONE_SECONDS, DST, NTP_SERVERS[0], NTP_SERVERS[1], NTP_SERVERS[2]);

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
        ntpSync = true;
    }
    else
    {
        Serial.println("Failed to synchronize time from NTP.");
        ntpSync = false;
        displayState = SHOW_NTP_FAILED;
    }
}
// NTP Synchronization ================

// Date & Time configuration ================
uint16_t h, m, s;
uint16_t year, month, day;
bool use12HourFormat = false;
unsigned long lastTimeUpdate = 0;
static bool flasher = false;
const char *monthNames[] = {
    "Invalid", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
const char *daysOfWeek[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
// Display buffers
char szClock[12];  // HH:MM:SS\0
char szTime[9];    // HH:MM\0
char szSeconds[4]; // SS\0
char szWDay[4];
char szDate[2];
char szMonth[2];
char Time[] = "00:00";
char Seconds[] = "00";
void initTime()
{
    use12HourFormat = getKeyValue("use12HourFormat", "0").toInt() == 1;
}
void updateClock()
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

    sprintf(szSeconds, "%02d", s);
    sprintf(szTime, "%02d%c%02d", h, (flasher ? ':' : ' '), m);
    sprintf(szClock, "%02d%c%02d%c%02d", h, (flasher ? ':' : ' '), m, (flasher ? ':' : ' '), s);
    flasher = !flasher;

    Seconds[1] = s % 10 + 48;
    Seconds[0] = s / 10 + 48;
    Time[4] = m % 10 + 48;
    Time[3] = m / 10 + 48;
    Time[1] = h % 10 + 48;
    Time[0] = h / 10 + 48;
}
void updateDate()
{
    time_t now = time(nullptr);
    struct tm *p_tm = localtime(&now);

    year = p_tm->tm_year + 1900;
    month = p_tm->tm_mon + 1;
    day = p_tm->tm_mday;
    String shortMonth = monthNames[month];
    String shortDay = daysOfWeek[p_tm->tm_wday];

    shortDay.toUpperCase();
    sprintf(szWDay, "%s", shortDay);
    sprintf(szDate, "%02d", day);
    sprintf(szMonth, "%02d", month);
}
String getDateString()
{
    time_t now = time(nullptr);
    struct tm *p_tm = localtime(&now);

    year = p_tm->tm_year + 1900;
    month = p_tm->tm_mon + 1;
    day = p_tm->tm_mday;
    String shortMonth = monthNames[month];
    String shortDay = daysOfWeek[p_tm->tm_wday];

    shortDay.toUpperCase();
    shortMonth.toUpperCase();
    // String text2 = shortDay + " " + day + " " + shortMonth + " " + year;
    String text2 = shortDay + " " + day + " " + shortMonth;

    return text2;
}
// Date & Time configuration ================

// Display configuration ================
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 5
#define CLK_PIN D5  // SCK
#define DATA_PIN D7 // MOSI
#define CS_PIN D8   // SS / CS
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

bool autoBrightness = true;
int brightness = 1;
int minBrightness = 0;
int maxBrightness = 3;
int minIntensity = 1023; // Dark
int maxIntensity = 800;  // Light
unsigned long stateMillis = 0;
unsigned long SHOW_CLOCK_DELAY_MS = 45000; // 45 seconds
unsigned long SHOW_DATE_DELAY_MS = 15000;  // 15 seconds
DisplayState lastDisplayState;
String msg = "";
bool isScrolling = false;
void initDisplay()
{
    autoBrightness = getKeyValue("auto_brightness", autoBrightness ? "1" : "0") == "1";
    brightness = getKeyValue("brightness", String(brightness).c_str()).toInt();
    minBrightness = getKeyValue("min_brightness", String(minBrightness).c_str()).toInt();
    maxBrightness = getKeyValue("max_brightness", String(maxBrightness).c_str()).toInt();
    SHOW_CLOCK_DELAY_MS = getKeyValue("SHOW_CLOCK_DELAY_MS", String(SHOW_CLOCK_DELAY_MS).c_str()).toInt();
    SHOW_DATE_DELAY_MS = getKeyValue("SHOW_DATE_DELAY_MS", String(SHOW_DATE_DELAY_MS).c_str()).toInt();

    P.begin();
    P.setInvert(false);
    P.setIntensity(brightness);
}
// void triggerSingleZoneDisplay()
// {
//     // Reassign Zone 0 to swallow all hardware modules (0 to 4)
//     P.setZone(0, 0, MAX_DEVICES - 1);
//     P.setFont(0, nullptr);
//     P.displayReset(0);
//     P.setIntensity(brightness);
//     P.displayClear();
// }
// void triggerSplitZoneDisplay()
// {
//     // Split them back out whenever you need to
//     P.setZone(0, 0, 1);
//     P.setZone(1, 2, 4);
//     P.setFont(0, smallerDigits);
//     P.setFont(1, smallDigits);
//     P.displayReset(0);
//     P.displayReset(1);
//     P.setIntensity(brightness);
// }
void scrollingDisplay(const char *text, uint16_t speed, uint16_t pause)
{
    isScrolling = true;
    Serial.println(text);
    P.displayClear();
    P.setFont(nullptr);
    P.displayText(text, PA_LEFT, speed, pause, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
}
void staticClockDisplay()
{
    isScrolling = false;
    updateClock();
    P.displayText(szClock, PA_CENTER, 75, 0, PA_NO_EFFECT);
    P.displayAnimate();
}
void updateDisplay()
{
    // displayAnimate handles non-blocking frame updates automatically
    if (isScrolling)
        if (P.displayAnimate())
            P.displayReset();

    // check update display every second
    if (millis() - lastTimeUpdate >= 1000)
    {
        lastTimeUpdate = millis();

        // CHECK LDR SENSOR FOR UPDATE AUTO BRIGHTNESS
        if (autoBrightness)
        {
            int ldrValue = analogRead(LDR_PIN);
            // Serial.printf("Analog (Light Intensity): %d\n", ldrValue);
            // ldr < 800 = 3
            // ldr > 1000 = 0
            // ldr < 800 + 100 = 2
            // else = 1
            brightness = ldrValue < maxIntensity ? maxBrightness : (ldrValue > minIntensity)     ? minBrightness
                                                               : (ldrValue < maxIntensity + 100) ? 2
                                                                                                 : 1;
            // Serial.printf("brightness: %d\n", brightness);
            P.setIntensity(brightness);
        }

        // CHECK displayState
        switch (displayState)
        {
        case SHOW_CONNECTION_SETUP:
            if (displayState != lastDisplayState)
            {
                Serial.println("SHOW_CONNECTION_SETUP");
                stateMillis = millis();

                msg = "INITIAL SETUP IP:" + WiFi.softAPIP().toString();
                scrollingDisplay(msg.c_str(), 50, 0);
                lastDisplayState = displayState;
            }
            if (millis() - stateMillis >= 30000)
            {
                stateMillis = millis();
                scrollingDisplay(msg.c_str(), 50, 0);
            }
            break;
        case SHOW_CONNECTION_FAILED:
            if (displayState != lastDisplayState)
            {
                Serial.println("SHOW_CONNECTION_FAILED");
                stateMillis = millis();

                msg = "CONNECTION FAILED | IP:" + WiFi.localIP().toString();
                scrollingDisplay(msg.c_str(), 50, 0);
                lastDisplayState = displayState;
            }
            if (millis() - stateMillis >= 30000)
            {
                stateMillis = millis();
                scrollingDisplay(msg.c_str(), 50, 0);
            }
            break;
        case SHOW_TIMEZONE_FAILED:
            if (displayState != lastDisplayState)
            {
                Serial.println("SHOW_TIMEZONE_FAILED");
                stateMillis = millis();

                msg = "TIMEZONE SYNC FAILED | Retry...";
                scrollingDisplay(msg.c_str(), 50, 0);
                lastDisplayState = displayState;
            }
            if (millis() - stateMillis >= 30000)
            {
                stateMillis = millis();
                scrollingDisplay(msg.c_str(), 50, 0);
            }
            break;
        case SHOW_NTP_FAILED:
            if (displayState != lastDisplayState)
            {
                Serial.println("SHOW_NTP_FAILED");
                stateMillis = millis();

                msg = "NTP SYNC FAILED | Retry...";
                scrollingDisplay(msg.c_str(), 50, 0);
                lastDisplayState = displayState;
            }
            if (millis() - stateMillis >= 30000)
            {
                stateMillis = millis();
                scrollingDisplay(msg.c_str(), 50, 0);
            }
            break;
        case SHOW_CLOCK:
            if (displayState != lastDisplayState)
            {
                Serial.println("SHOW_CLOCK");
                stateMillis = millis();

                P.setFont(smallDigits);
                staticClockDisplay();
                lastDisplayState = displayState;
            }

            staticClockDisplay();

            if (millis() - stateMillis >= SHOW_CLOCK_DELAY_MS)
            {
                displayState = SHOW_DATE;
            }
            break;
        case SHOW_DATE:
            if (displayState != lastDisplayState)
            {
                Serial.println("SHOW_DATE");
                stateMillis = millis();

                updateDate();

                // msg = String(szWDay) + " " + String(szDate);
                msg = getDateString();
                scrollingDisplay(msg.c_str(), 50, 2000);
                lastDisplayState = displayState;
            }
            if (millis() - stateMillis >= 5000)
            {
                displayState = SHOW_TEMP_HUM;
            }
            break;
        case SHOW_TEMP_HUM:
            if (displayState != lastDisplayState)
            {
                Serial.println("SHOW_TEMP_HUM");
                stateMillis = millis();

                msg = getTempHumString();
                scrollingDisplay(msg.c_str(), 50, 2000);
                lastDisplayState = displayState;
            }
            if (millis() - stateMillis >= 6000)
            {
                displayState = SHOW_CLOCK;
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
    html.replace("{brightness}", String(brightness) + "/" + String(maxBrightness));
    html.replace("{brightness_mode}", String(autoBrightness ? "Auto" : "Manual"));
    html.replace("{brightness_manual}", String(autoBrightness ? "" : "selected"));
    html.replace("{brightness_auto}", String(autoBrightness ? "selected" : ""));
    html.replace("{min_brightness}", String(minBrightness));
    html.replace("{max_brightness}", String(maxBrightness));
    html.replace("{brightness_value}", String(brightness));
    html.replace("{set_brightness}", String(autoBrightness ? "disabled" : ""));
    html.replace("{time_format}", String(use12HourFormat ? "12 hour (AM/PM)" : "24 hour"));
    html.replace("{time_format_24}", String(use12HourFormat ? "" : "selected"));
    html.replace("{time_format_12}", String(use12HourFormat ? "selected" : ""));
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
void handleSetBrightnessMode()
{
    if (server.hasArg("brightness_mode"))
    {
        autoBrightness = server.arg("brightness_mode").toInt() == 1;
        setKeyValue("auto_brightness", String(autoBrightness ? "1" : "0").c_str());
    }
    server.sendHeader("Location", "/");
    server.send(303);
}
void handleSetBrightness()
{
    if (server.hasArg("brightness"))
    {
        brightness = server.arg("brightness").toInt();
        P.setIntensity(brightness);
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
    server.on("/set_brightness_mode", handleSetBrightnessMode);
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

    initDisplay();
    displayState = SHOW_CONNECTION_SETUP;
    initLittleFS();
    deviceLocation = getKeyValue("device_location", "");
    initLedIndicator();
    initAHTSensor();
    initGeoLocation();
    initTime();
    // setupEye();

    initConnection([](bool connected)
                   {
        if (!connected)
            {
                Serial.println("Connection Failed !");
                displayState = SHOW_CONNECTION_SETUP;
            } });
    // Setup mDNS for local network access
    initDNS();
}

void loop()
{
    wifiManager.process();
    updateLedIndicator();
    updateAHTSensor();
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
            if (geoSync && ntpSync)
            {
                displayState = SHOW_CLOCK;
            }
            currLedState = CONNECTED;
        } });
    // This section will running after the connection established !
    if (isConnected)
    {
        // runBlynk();
        runWebServer();
        
        if (!geoSync && millis() - lastGeoSync >= RETRY_GEOSYNC_MS)
            fetchGeolocation();
        // Retry Sync to NTP Server when FAILED every interval time
        if (geoSync && !ntpSync && millis() - lastNTPSync >= RETRY_NTPSYNC_MS)
            syncTimeFromNTP();
        // Sync to NTP Server every interval time
        if (geoSync && ntpSync && millis() - lastNTPSync >= INTERVAL_NTPSYNC_MS)
            syncTimeFromNTP();
    }
}
