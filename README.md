# ESP32httpOTA

[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)](https://github.com/voknetral/ESP32httpOTA)
[![Platform](https://img.shields.io/badge/platform-ESP32-green.svg)](https://github.com/espressif/arduino-esp32)
[![License](https://img.shields.io/badge/license-MIT-orange.svg)](LICENSE)
[![Arduino](https://img.shields.io/badge/Arduino_IDE-compatible-00979D.svg)](https://www.arduino.cc/)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-compatible-FF7F00.svg)](https://platformio.org)

**Library OTA (Over-The-Air) firmware update untuk ESP32 via HTTP/HTTPS.**

Library ringan untuk mengecek dan menerapkan update firmware dari **server HTTP/HTTPS manapun** — GitHub, VPS, S3, Firebase, server lokal LAN, atau URL apapun yang menyediakan file JSON manifest dan file `.bin`.

---

## Requirements

- **Board**: ESP32 (semua varian)
- **Arduino Core**: [arduino-esp32](https://github.com/espressif/arduino-esp32)
- **Library**:
  - [ArduinoJson](https://arduinojson.org/) **v7+** (install via Library Manager)

---

## Instalasi

### Arduino IDE

**Cara 1 — ZIP:**
1. Download repository sebagai ZIP
2. Buka Arduino IDE → **Sketch → Include Library → Add .ZIP Library**
3. Pilih file ZIP yang didownload

**Cara 2 — Manual:**
1. Copy folder `ESP32httpOTA/` ke `Documents/Arduino/libraries/`
2. Restart Arduino IDE

### PlatformIO

Tambahkan di `platformio.ini`:

```ini
lib_deps =
    voknetral/ESP32httpOTA
    bblanchon/ArduinoJson@^7
```

---

## Quick Start

### HTTPS (GitHub, S3, Cloud Server)

```cpp
#define OTA_DEBUG                 // Untuk mengaktifkan log debug (opsional)
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP32httpOTA.h>

// Buat instance dengan versi firmware saat ini dan URL manifest
ESP32httpOTA ota("1.0.0", "https://raw.githubusercontent.com/user/repo/main/version.json");

void setup() {
    Serial.begin(115200);

    // Inisialisasi WiFi
    WiFi.begin("SSID", "PASSWORD");
    while (WiFi.status() != WL_CONNECTED) delay(500);
    Serial.println("WiFi OK!");

    // Konfigurasi client HTTPS
    WiFiClientSecure client;
    client.setInsecure();          // atau client.setCACert(rootCA) untuk verifikasi

    // Cek dan menjalankan update
    OTAResult result = ota.run(client);
    Serial.println(ESP32httpOTA::resultToString(result));

    // Restart jika update berhasil
    if (result == OTA_SUCCESS) {
        Serial.println("Update berhasil! Restart...");
        delay(1000);
        ESP.restart();
    }
}

void loop() {
    // Program utama
}
```

### HTTP (Server Lokal / LAN)

```cpp
#include <WiFi.h>
#include <ESP32httpOTA.h>

ESP32httpOTA ota("1.0.0", "http://192.168.1.100/version.json");

void setup() {
    Serial.begin(115200);
    WiFi.begin("SSID", "PASSWORD");
    while (WiFi.status() != WL_CONNECTED) delay(500);

    WiFiClient client;                       // HTTP biasa, tanpa SSL
    OTAResult result = ota.run(client);

    if (result == OTA_SUCCESS) {
        Serial.println("Update berhasil! Restart...");
        delay(1000);
        ESP.restart();
    }
}

void loop() {
    // Program utama
}
```

---

## Setup Server

### 1. Buat File Manifest (`version.json`)

Buat file JSON sederhana di server:

```json
{
    "version": "1.1.0",
    "firmware": "https://yourserver.com/firmware.bin"
}
```

| Field | Deskripsi |
|-------|-----------|
| `version` | Versi firmware terbaru (format `major.minor.patch`) |
| `firmware` | URL langsung ke file `.bin` yang sudah dikompilasi |

### 2. Export Binary dari Arduino IDE

```
Arduino IDE → Sketch → Export Compiled Binary
```

File `.bin` akan ada di folder sketch. Upload ke server bersama `version.json`.

---

## API Reference

### Constructor

```cpp
ESP32httpOTA ota("1.0.0", "https://yourserver.com/version.json");
```

| Parameter | Tipe | Deskripsi |
|-----------|------|-----------|
| `version` | `const char*` | Versi firmware saat ini (`"major.minor.patch"`) |
| `manifestUrl` | `const char*` | URL ke file `version.json` di server |

---

### `run(WiFiClientSecure& client)` → `OTAResult`
### `run(WiFiClient& client)` → `OTAResult`

Fungsi utama. Cek manifest di server, bandingkan versi, dan update jika ada versi baru.

**Alur kerja:**
1. `GET version.json` dari server
2. Parse JSON → ambil `version` dan `firmware` URL
3. Bandingkan versi dengan Semantic Versioning
4. Jika versi baru lebih tinggi → download `.bin` → flash ke ESP32

```cpp
// HTTPS
WiFiClientSecure secureClient;
secureClient.setInsecure();
OTAResult result = ota.run(secureClient);

// HTTP
WiFiClient httpClient;
OTAResult result = ota.run(httpClient);
```

> **Warning:** Library TIDAK restart device otomatis. Cek `OTA_SUCCESS` dan panggil `ESP.restart()` sendiri.

---

### `currentVersion()` → `const char*`

Return versi firmware saat ini yang di-set di constructor.

```cpp
Serial.printf("Firmware v%s\n", ota.currentVersion());
```

---

### `resultToString(OTAResult result)` → `const char*` *(static)*

Konversi result code ke string yang mudah dibaca.

```cpp
OTAResult result = ota.run(client);
Serial.println(ESP32httpOTA::resultToString(result));
// Output: "Success" / "Already up to date" / "HTTP request failed" / dll
```

---

## Result Codes

| Code | Nilai | Deskripsi |
|------|-------|-----------|
| `OTA_SUCCESS` | 0 | Update berhasil di-flash, siap restart |
| `OTA_NO_UPDATE` | 1 | Firmware sudah up to date |
| `OTA_HTTP_ERROR` | 2 | Gagal konek ke server |
| `OTA_JSON_ERROR` | 3 | JSON invalid atau field hilang |
| `OTA_UPDATE_FAILED` | 4 | Download atau flash gagal |

---

## Debug Mode

Library **tidak menghasilkan output serial** secara default. Untuk mengaktifkan log debug:

```cpp
#define OTA_DEBUG          // HARUS sebelum #include
#include <ESP32httpOTA.h>
```

**Contoh output:**

```
[OTA] Checking manifest: https://yourserver.com/version.json
[OTA] Manifest received (89 bytes)
[OTA] Current: v1.0.0 | Latest: v1.1.0
[OTA] Update available: v1.0.0 -> v1.1.0
[OTA] Downloading: https://yourserver.com/firmware.bin
[OTA] Progress: 10%
[OTA] Progress: 20%
...
[OTA] Progress: 100%
[OTA] Update successful!
```

---

## Penting: Kode OTA Harus Ada di Setiap Firmware

Setiap firmware baru yang di-upload ke server **harus menyertakan kode ESP32httpOTA**. Jika tidak, device yang sudah di-update tidak akan bisa menerima update berikutnya dan harus di-flash manual via USB.

Pastikan juga untuk **mengubah string versi** di constructor sesuai versi firmware yang baru:

```cpp
// Firmware lama (v1.0.0)
ESP32httpOTA ota("1.0.0", "https://server.com/version.json");

// Firmware baru (v1.1.0) — ubah versi di sini
ESP32httpOTA ota("1.1.0", "https://server.com/version.json");
```

Jika versi tidak diubah, library akan menganggap firmware sudah up to date dan tidak akan melakukan update.

---

## FAQ

### Apakah library ini support ESP8266?
Tidak. Library ini khusus untuk **ESP32** karena menggunakan `HTTPUpdate` dari arduino-esp32 core.

### Harus pakai HTTPS?
Tidak. Library support HTTP biasa dengan `WiFiClient`. Tapi untuk production disarankan HTTPS.

### Bisa pakai Ethernet selain WiFi?
Bisa. Selama kamu punya `Client` yang terkoneksi, library bisa dipakai.

### Apakah aman jika update gagal di tengah jalan?
Ya. ESP32 menggunakan dual-partition scheme. Jika flash gagal, device tetap boot dari partisi yang lama.

### Kenapa library tidak restart sendiri?
Best practice: library tidak boleh mengambil kontrol restart. User mungkin perlu menyimpan data, menutup koneksi, atau memberi notifikasi sebelum restart.

---

## License

MIT License — lihat [LICENSE](LICENSE) untuk detail.

---

**Made by [voknetral](https://github.com/voknetral)**
