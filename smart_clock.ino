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
#include "Font_Data_2.h"
#include "index_page.h"
#include "ota_page.h"

#define SDA_PIN 4     // D2
#define SCL_PIN 5     // D1
uint8_t LDR_PIN = 17; // A0

String deviceLocation = "";
String version = "1.1.0";

enum DisplayState
{
    SHOW_CONNECTION_SETUP,
    SHOW_CONNECTION_FAILED,
    SHOW_TIMEZONE_FAILED,
    SHOW_NTP_FAILED,
    SHOW_CLOCK,
    SHOW_WDAY,
    SHOW_DATE,
    SHOW_TEMP_HUM,
    SHOW_CUSTOM_TEXT,
};
DisplayState displayState = SHOW_CLOCK;
// Display buffer
char buffer[40];

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
}
// TEMP & HUM ===========================

// GEOLOCATION & TIMEZONE ================
// Timezone in seconds: Auto-detected from geolocation during startup
// Fallback: 0 = UTC (will be overridden by geolocation)
int32_t TIMEZONE_SECONDS = 7 * 3600; // Adjust timezone offset (e.g., GMT+7 = 7 * 3600)
// Geolocation API endpoint
const char *GEOLOCATION_API = "http://ip-api.com/json/?fields=country,city,lat,lon,timezone,offset";
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
    // geoSync = getKeyValue("geo_sync", String(geoSync ? "1" : "0").c_str()).toInt() == 1;
    TIMEZONE_SECONDS = getKeyValue("TIMEZONE_SECONDS", String(TIMEZONE_SECONDS).c_str()).toInt();
    // Serial.printf("Last Geolocation Sync: %s | Timezone offset (seconds): %d\n", geoSync == 1 ? "TRUE" : "FALSE", TIMEZONE_SECONDS);
}
void fetchGeolocation()
{
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
}
// GEOLOCATION & TIMEZONE ================

// NTP Synchronization ================
const int DST = 0;
// NTP servers for time synchronization
const char *NTP_SERVERS[] = {"pool.ntp.org", "time.nist.gov", "asia.pool.ntp.org", "time.google.com"};
unsigned long lastNTPSync = 0;
const unsigned long RETRY_NTPSYNC_MS = 10000;         // retry every 10 seconds when sync FAILED
const unsigned long INTERVAL_NTPSYNC_MS = 60000 * 60; // every 1 hours sync to NTP server
void syncTimeFromNTP()
{
    Serial.println("Synchronizing time from NTP...");
    Serial.print("Using timezone offset (seconds): ");
    Serial.println(TIMEZONE_SECONDS);

    lastNTPSync = millis();

    configTime(TIMEZONE_SECONDS, DST, NTP_SERVERS[0], NTP_SERVERS[1], NTP_SERVERS[2]);

    // Wait for time to be set (max 10 seconds)
    // uint32_t startTime = millis();
    // time_t now = time(nullptr);

    // while (now < 24 * 3600 && (millis() - startTime) < 10000)
    // {
    //     delay(100);
    //     now = time(nullptr);
    // }

    // if (now > 24 * 3600)
    // {
    //     Serial.println("Time synchronized successfully");
    //     // Print the synchronized time
    //     time_t syncTime = time(nullptr);
    //     struct tm *p_tm = localtime(&syncTime);
    //     Serial.print("Fetched UTC time: ");
    //     Serial.print(p_tm->tm_year + 1900);
    //     Serial.print("-");
    //     Serial.print(p_tm->tm_mon + 1);
    //     Serial.print("-");
    //     Serial.print(p_tm->tm_mday);
    //     Serial.print(" ");
    //     Serial.print(p_tm->tm_hour);
    //     Serial.print(":");
    //     Serial.print(p_tm->tm_min);
    //     Serial.print(":");
    //     Serial.println(p_tm->tm_sec);
    // }
    // else
    // {
    //     Serial.println("Failed to synchronize time from NTP.");
    // }
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
const char *daysOfWeekID[] = {"Ahad", "Senin", "Selasa", "Rabu", "Kamis", "Jumat", "Sabtu"};
void initTime()
{
    use12HourFormat = getKeyValue("use12HourFormat", "0").toInt() == 1;
}
// Date & Time configuration ================

// Display configuration ================
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 8
#define CLK_PIN D5  // SCK
#define DATA_PIN D7 // MOSI
#define CS_PIN D8   // SS / CS
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

bool autoBrightness = false;
int brightness = 3;
int minBrightness = 0;
int maxBrightness = 15;
int minIntensity = 1023; // Dark
int maxIntensity = 800;  // Light
unsigned long clockTimer = 0, clockBlinkingTimer = 0, autoBrightnessTimer = 0;
unsigned long SHOW_CLOCK_DELAY_MS = 30000; // 30 seconds
std::vector<std::string> customText = {
    "",
    "",
    ""};
String customText0 = "Selamat datang tamu Rasulullah SAW !";
String customText1 = "1448 H / 2026 M";
String customText2 = "PAUD Az-Zahra";
int customTextCount = sizeof(customText) / sizeof(customText[0]);
int customTextIdx = 0;
int steps = 1;
// Colon override: top 1 row off, dots rows 1-2, gap row 3, dots rows 4-5, bottom 2 rows off
uint8_t colonChar[] = {2, 0x36, 0x36};
uint8_t narrowBlank[] = {2, 0x00, 0x00};      // same width as colon, for blink-off
uint8_t smallA_chr[] = {3, 0x78, 0x14, 0x78}; // 3x5 small A (AM indicator)
uint8_t smallP_chr[] = {3, 0x7C, 0x14, 0x0C}; // 3x5 small P (PM indicator)
void initDisplay()
{
    autoBrightness = getKeyValue("auto_brightness", autoBrightness ? "1" : "0") == "1";
    brightness = getKeyValue("brightness", String(brightness).c_str()).toInt();
    minBrightness = getKeyValue("min_brightness", String(minBrightness).c_str()).toInt();
    maxBrightness = getKeyValue("max_brightness", String(maxBrightness).c_str()).toInt();
    SHOW_CLOCK_DELAY_MS = getKeyValue("SHOW_CLOCK_DELAY_MS", String(SHOW_CLOCK_DELAY_MS).c_str()).toInt();
    customText0 = getKeyValue("custom_text0", customText0.c_str());
    customText1 = getKeyValue("custom_text1", customText1.c_str());
    customText2 = getKeyValue("custom_text2", customText2.c_str());
    customText[0] = customText0.c_str();
    customText[1] = customText1.c_str();
    customText[2] = customText2.c_str();

    P.begin();
    P.setInvert(false);
    P.setIntensity(brightness);
    P.addChar(':', colonChar);
    P.addChar('', narrowBlank);
    P.addChar('', smallA_chr);
    P.addChar('', smallP_chr);
    P.displayClear();
    // P.setFont(smallDigits);
    // P.setFont(myFont);

    displayState = SHOW_CLOCK;
}
void updateDisplay()
{
    if (millis() - autoBrightnessTimer >= 1000)
    {
        autoBrightnessTimer = millis();
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
    }

    if (P.displayAnimate())
    {
        switch (displayState)
        {
        case SHOW_CLOCK:
            if (steps == 1)
            {
                Serial.println("SHOW_CLOCK-IN");
                steps = 2;
                clockTimer = millis();
                getClockString(buffer);
                Serial.println(buffer);
                P.displayText(buffer, PA_CENTER, 50, 500, PA_SCROLL_UP, PA_NO_EFFECT);
            }
            else if (steps == 2)
            {
                // Update every second for blinking effect
                if (millis() - clockBlinkingTimer >= 1000)
                {
                    Serial.println("SHOW_CLOCK-TACK");
                    clockBlinkingTimer = millis();
                    getClockString(buffer);
                    Serial.println(buffer);
                    P.print(buffer);
                }
            }
            if (millis() - clockTimer >= SHOW_CLOCK_DELAY_MS)
            {
                Serial.println("SHOW_CLOCK-OUT");
                steps = 1;
                displayState = SHOW_WDAY;
                getClockString(buffer);
                Serial.println(buffer);
                P.displayText(buffer, PA_CENTER, 50, 100, PA_NO_EFFECT, PA_SCROLL_UP);
            }
            break;

        case SHOW_WDAY:
            Serial.println("SHOW_WDAY");
            getWeekDay(buffer);
            Serial.println(buffer);
            P.displayText(buffer, PA_CENTER, 75, 1500, PA_SCROLL_UP, PA_SCROLL_UP);
            displayState = SHOW_DATE;
            break;

        case SHOW_DATE:
            Serial.println("SHOW_DATE");
            getDateString(buffer);
            Serial.println(buffer);
            P.displayText(buffer, PA_CENTER, 50, 2000, PA_SCROLL_UP, PA_SCROLL_UP);
            displayState = SHOW_TEMP_HUM;
            break;

        case SHOW_TEMP_HUM:
            Serial.println("SHOW_TEMP_HUM");
            getWeatherString(buffer);
            Serial.println(buffer);
            if (String(buffer) != "Sensor Error")
            {
                if (MAX_DEVICES > 4)
                {
                    P.displayText(buffer, PA_CENTER, 50, 3000, PA_SCROLL_UP, PA_SCROLL_UP);
                }
                else 
                {
                    P.displayText(buffer, PA_CENTER, 50, 50, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
                }
            }
            displayState = SHOW_CUSTOM_TEXT;
            break;

        case SHOW_CUSTOM_TEXT:
            if (customTextIdx < 3)
            {
                Serial.println("SHOW_CUSTOM_TEXT");
                Serial.println(customText[customTextIdx].c_str());
                if (customText[customTextIdx] != "")
                {
                    P.displayText(customText[customTextIdx].c_str(), PA_CENTER, 50, 50, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
                }
                customTextIdx++;
            }
            else
            {
                customTextIdx = 0; // reset index
                displayState = SHOW_CLOCK;
            }
            break;
        default:
            P.displayReset();
            break;
        }
    }
}
void getClockString(char *buffer)
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        strcpy(buffer, "00:00:00");
        return;
    }
    if (use12HourFormat)
    {
        h = timeinfo.tm_hour % 12;
        h = h == 0 ? 12 : h;
    }
    else
    {
        h = timeinfo.tm_hour;
    }
    m = timeinfo.tm_min;
    s = timeinfo.tm_sec;
    // Alternates the colon blinking every second
    if (MAX_DEVICES > 4)
    {
        if (use12HourFormat)
        {
            String ampm = (timeinfo.tm_hour < 12) ? "am" : "pm";
            sprintf(buffer, "%02d%c%02d%c%02d%s", h, (flasher ? ':' : ' '), m, (flasher ? ':' : ' '), s, ampm);
        }
        else
        {
            sprintf(buffer, "%02d%c%02d%c%02d", h, (flasher ? ':' : ' '), m, (flasher ? ':' : ' '), s);
        }
    }
    else
    {
        if (use12HourFormat)
        {
            char ampm = (timeinfo.tm_hour < 12) ? '' : '';
            sprintf(buffer, "%d%c%02d%c", h, (flasher ? ':' : ' '), m, ampm);
        }
        else
        {
            sprintf(buffer, "%d%c%02d", h, (flasher ? ':' : ' '), m);
        }
    }
    flasher = !flasher;
}
void getWeekDay(char *buffer)
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        strcpy(buffer, "No Day");
        return;
    }
    sprintf(buffer, "%s", daysOfWeekID[timeinfo.tm_wday]);
}
void getDateString(char *buffer)
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        strcpy(buffer, "No Date");
        return;
    }
    year = timeinfo.tm_year + 1900;
    month = timeinfo.tm_mon + 1;
    day = timeinfo.tm_mday;
    String shortMonth = monthNames[month];
    if (MAX_DEVICES > 4)
    {
        // strftime(buffer, 20, "%A, %b %d", &timeinfo); // Example: "Monday, Sep 01"
        sprintf(buffer, "%d %s %04d", day, shortMonth, year); // 1 Jun 2026
    }
    else
    {
        sprintf(buffer, "%d %s", day, shortMonth); // 1 Jun
    }
}
void getWeatherString(char *buffer)
{
    sensors_event_t humidityEvent, tempEvent;

    if (!aht.getEvent(&humidityEvent, &tempEvent))
    {
        strcpy(buffer, "Sensor Error");
    }
    else
    {
        temperature = tempEvent.temperature + offsetTemp;
        humidity = humidityEvent.relative_humidity + offsetHum;
        snprintf(buffer, 30, "T:%.1fC  H:%.0f%%", temperature, humidity); // Output pattern: T:26.5C H:65%
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
    html.replace("{custom_text0}", String(customText0));
    html.replace("{custom_text1}", String(customText1));
    html.replace("{custom_text2}", String(customText2));
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
void handleSetCustomText()
{
    if (server.hasArg("custom_text0"))
    {
        customText0 = server.arg("custom_text0").c_str();
        customText[0] = customText0.c_str();
        setKeyValue("custom_text0", customText0.c_str());
    }
    if (server.hasArg("custom_text1"))
    {
        customText1 = server.arg("custom_text1").c_str();
        customText[1] = customText1.c_str();
        setKeyValue("custom_text1", customText1.c_str());
    }
    if (server.hasArg("custom_text2"))
    {
        customText2 = server.arg("custom_text2").c_str();
        customText[2] = customText2.c_str();
        setKeyValue("custom_text2", customText2.c_str());
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
    server.on("/set_custom_text", handleSetCustomText);
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
    initLittleFS();
    deviceLocation = getKeyValue("device_location", "");
    initLedIndicator();
    initAHTSensor();
    initGeoLocation();
    initTime();

    initWiFiConnection(onConnection, onConnectingResult);
    // Setup mDNS for local network access
    initDNS();
}

void loop()
{
    wifiManager.process();
    updateLedIndicator();
    runDNS();
    updateDisplay();
    checkWiFiConnection(onConnected);
    // This section will running after the connection established !
    if (isWiFiConnected)
    {
        // runBlynk();
        runWebServer();

        // if (!geoSync && millis() - lastGeoSync >= RETRY_GEOSYNC_MS)
        //     fetchGeolocation();
        // Retry Sync to NTP Server when FAILED every interval time
        // if (geoSync && !ntpSync && millis() - lastNTPSync >= RETRY_NTPSYNC_MS)
        //     syncTimeFromNTP();
        // Sync to NTP Server every interval time
        if (millis() - lastNTPSync >= INTERVAL_NTPSYNC_MS)
            syncTimeFromNTP();
    }
}

void onConnection()
{
    // onConnecting...
    Serial.println("On Connecting...");
    P.displayClear();
    P.displayText("WiFi...", PA_LEFT, 60, 1000, PA_NO_EFFECT, PA_SCROLL_LEFT);
}
void onConnectingResult(bool connected)
{
    if (!connected)
    {
        Serial.println("Connection Failed !");
        P.displayClear();
        // P.print("Connection Failed !");
        P.displayText("Connection Failed !", PA_LEFT, 60, 1000, PA_NO_EFFECT, PA_SCROLL_LEFT);
    }
    else
    {
        Serial.println("Yee hay connected !");
    }
}
void onConnected()
{
    // This section only running once, after connection establish !
    // This like setup()
    Serial.println("This only running once, when WiFi connected !");
    currLedState = CONNECTED;
    // initBlynk();
    initWebserver();
    fetchGeolocation();
    syncTimeFromNTP();
}