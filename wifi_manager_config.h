// #include <WiFiManager.h>

// // WIFI MANAGER =========================
// String apName = "SmartClock";
// WiFiManager wifiManager;
// bool isWiFiConnected = false;
// unsigned long lastWiFiCheckTime = 0;
// // unsigned long portalStartTime = 0;
// // const unsigned long TIMEOUT_MS = 5000; // 5 seconds
// void initWiFiConnection(const std::function<void()> &onConnecting, const std::function<void(bool)> &callbackResult)
// {
//     wifiManager.setConnectTimeout(15);

//     // Enable non-blocking mode
//     wifiManager.setConfigPortalBlocking(false);

//     onConnecting();

//     // Start the asynchronous connection attempt
//     apName = apName + "-" + String(ESP.getChipId(), HEX);
//     if (!wifiManager.autoConnect(apName.c_str(), ""))
//     {
//         // Serial.println("AutoConnect Failed | Callback => FALSE");
//         delay(1000);
//         callbackResult(false);
//         return;
//     }

//     callbackResult(true);

//     // Mark when we started trying to connect
//     // portalStartTime = millis();
// }
// void checkWiFiConnection(const std::function<void()> &onConnected)
// {
//     if (!isWiFiConnected && millis() - lastWiFiCheckTime >= 1000)
//     {
//         lastWiFiCheckTime = millis();
//         if (WiFi.status() == WL_CONNECTED && WiFi.localIP() == IPAddress(0, 0, 0, 0))
//         {
//             Serial.println("\n[ERROR] Connected but IP is 0.0.0.0! Restarting...");
//             delay(2000);
//             ESP.restart();
//         }
//         if (WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0))
//         {
//             Serial.print("\n[SUCCESS] Connected! IP: ");
//             Serial.println(WiFi.localIP());

//             delay(1000);

//             isWiFiConnected = true;

//             onConnected();
//         }
//         // if (WiFi.status() != WL_CONNECTED && millis() - portalStartTime >= TIMEOUT_MS)
//         // {
//         //     Serial.println("\n[TIMEOUT] Connection failed to establish. Restarting...");
//         //     delay(2000);
//         //     ESP.restart();
//         // }
//     }
// }
// // WIFI MANAGER =========================
