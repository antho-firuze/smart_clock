#include <LittleFS.h>
#include <ArduinoJson.h>

// LittleFS =============================
const char *CONFIG_PATH = "/config.json";
void initLittleFS()
{
    if (!LittleFS.begin())
    {
        Serial.println("Failed to load LittleFS!");
        return;
    }
    Serial.println("LittleFS Successfully Loaded.");
}
// Fungsi untuk MENYIMPAN Key-Value
bool setKeyValue(const char *key, const char *value)
{
    StaticJsonDocument<512> doc; // Sesuaikan ukuran byte dengan kebutuhan data Anda

    // 1. Baca file yang sudah ada agar data lama tidak terhapus
    if (LittleFS.exists(CONFIG_PATH))
    {
        File file = LittleFS.open(CONFIG_PATH, "r");
        if (file)
        {
            deserializeJson(doc, file);
            file.close();
        }
    }

    // 2. Tambah atau update pasangan key-value
    doc[key] = value;

    // 3. Tulis kembali data terbaru ke dalam file (Mode "w" akan menimpa file lama)
    File file = LittleFS.open(CONFIG_PATH, "w");
    if (!file)
    {
        return false; // Gagal membuka file
    }

    // serializeJson(doc, file) langsung menulis ke file tanpa memakan banyak RAM
    if (serializeJson(doc, file) == 0)
    {
        file.close();
        return false; // Gagal menulis
    }

    file.close();
    Serial.println("Key-Value pair saved successfully.");
    return true; // Sukses
}
// Fungsi untuk MEMBACA Key-Value
String getKeyValue(const char *key, const char *defaultValue = "")
{
    if (!LittleFS.exists(CONFIG_PATH))
    {
        return defaultValue;
    }

    File file = LittleFS.open(CONFIG_PATH, "r");
    if (!file)
    {
        return defaultValue;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error)
    {
        return defaultValue;
    }

    // Jika key tidak ditemukan, kembalikan nilai default
    if (!doc.containsKey(key))
    {
        return defaultValue;
    }

    return doc[key].as<String>();
}
// LittleFS =============================
