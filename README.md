# Smart Clock

An ESP/Arduino-based smart clock system equipped with a local web dashboard, captive portal/DNS capabilities, Over-The-Air (OTA) updates, and Blynk IoT integration.

---

## Features

* **IoT Dashboard Integration**: Connects seamlessly with Blynk (`blynk_config.h`) for remote monitoring and control.
* **Web Configuration Interface**: Embedded web server providing an intuitive user control page (`index.html`, `index_page.h`).
* **Captive Portal & DNS Support**: Local access and easy Wi-Fi configuration via DNS service setup (`dns_config.h`).
* **Over-The-Air (OTA) Updates**: Update firmware wirelessly directly through the web UI (`ota_page.h`).
* **LittleFS File System**: Efficient storage management for web assets and device configuration (`littleFS_config.h`).
* **Custom Display & Status**: Custom font rendering support (`Font_Data.h`) alongside visual hardware status feedback (`led_indicator.h`).

---

## Repository Structure

```text
smart_clock/
├── .gitignore
├── Font_Data.h        # Display font character data
├── blynk_config.h     # Blynk IoT platform configuration
├── dns_config.h       # Captive portal & DNS server settings
├── index.html         # Web UI interface markup
├── index_page.h       # Embedded web page header source
├── led_indicator.h    # Status LED control routines
├── littleFS_config.h  # Flash file system (LittleFS) configuration
├── ota_page.h         # OTA firmware upload web page
└── sketch.yaml        # Arduino CLI / IDE project configuration

```

---

## Hardware & Dependencies

* **Microcontroller**: ESP8266 or ESP32
* **Storage**: LittleFS (ensure flash partitioning is configured for SPIFFS/LittleFS in your IDE)
* **Libraries**:
* [Blynk](https://www.google.com/search?q=https://github.com/blynkkk/blynk-library)
* `DNSServer`
* `ESPAsyncWebServer` / `WebServer` (or equivalent depending on core setup)



---

## Getting Started

1. **Clone the Repository**:
```bash
git clone [https://github.com/antho-firuze/smart_clock.git](https://github.com/antho-firuze/smart_clock.git)

```


2. **Configure Settings**:
* Update `blynk_config.h` with your Blynk auth tokens and network credentials.
* Adjust display or pin definitions in `led_indicator.h` and `Font_Data.h` if necessary for your hardware layout.


3. **Upload Files**:
* Flash the project via Arduino IDE or Arduino CLI (`sketch.yaml` handles CLI build settings).
* Upload data files to LittleFS if required by your environment setup.


4. **Access the Web Dashboard**:
* Connect to the device's access point or IP address on your local network to configure options or upload new firmware via OTA.



---

## License

This project is open-source. Feel free to modify and adapt it for your own hardware builds.
