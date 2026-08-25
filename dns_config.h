#include <ESP8266mDNS.h>

// Local DNS name for OTA updates =======
String localDNS = "smartclock";
void initDNS()
{
    // Get the unique chip ID
    uint32_t chipId = ESP.getChipId();
    localDNS = localDNS + "-" + String(chipId, HEX);

    if (MDNS.begin(localDNS.c_str()))
    {
        Serial.println("Local DNS started: http://" + localDNS + ".local");
    }
    else
    {
        Serial.println("Error setting up MDNS responder!");
    }

    MDNS.addService("http", "tcp", 80);
}
void runDNS()
{
    MDNS.update();
}
// Local DNS name for OTA updates =======
