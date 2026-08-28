#include <WiFiManager.h>

// WIFI MANAGER =========================
String apName = "SmartClock";
WiFiManager wifiManager;
bool isConnected = false;
unsigned long lastWiFiCheckTime = 0;
// unsigned long portalStartTime = 0;
// const unsigned long TIMEOUT_MS = 5000; // 5 seconds 
void initConnection(const std::function<void(bool)> &callback)
{
    wifiManager.setConnectTimeout(15);

    // Enable non-blocking mode
    wifiManager.setConfigPortalBlocking(false);

    // Start the asynchronous connection attempt
    apName = apName + "-" + String(ESP.getChipId(), HEX);
    if (!wifiManager.autoConnect(apName.c_str(), ""))
    {
        // Serial.println("AutoConnect Failed | Callback => FALSE");
        delay(1000);
        callback(false);
    }

    // Mark when we started trying to connect
    // portalStartTime = millis();
}
void checkConnection(const std::function<void(bool)> &callback)
{
    if (!isConnected && millis() - lastWiFiCheckTime >= 1000)
    {
        lastWiFiCheckTime = millis();
        if (WiFi.status() == WL_CONNECTED && WiFi.localIP() == IPAddress(0, 0, 0, 0))
        {
            Serial.println("\n[ERROR] Connected but IP is 0.0.0.0! Restarting...");
            delay(2000);
            ESP.restart();
        }
        if (WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0))
        {
            Serial.print("\n[SUCCESS] Connected! IP: ");
            Serial.println(WiFi.localIP());

            delay(1000);

            isConnected = true;

            callback(true);
        }
        // if (WiFi.status() != WL_CONNECTED && millis() - portalStartTime >= TIMEOUT_MS)
        // {
        //     Serial.println("\n[TIMEOUT] Connection failed to establish. Restarting...");
        //     delay(2000);
        //     ESP.restart();
        // }
    }
}
// WIFI MANAGER =========================
