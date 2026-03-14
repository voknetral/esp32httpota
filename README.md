# ESP32httpOTA

[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)](https://github.com/voknetral/ESP32httpOTA)
[![Platform](https://img.shields.io/badge/platform-ESP32-green.svg)](https://github.com/espressif/arduino-esp32)
[![License](https://img.shields.io/badge/license-MIT-orange.svg)](LICENSE)
[![Arduino](https://img.shields.io/badge/Arduino_IDE-compatible-00979D.svg)](https://www.arduino.cc/)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-compatible-FF7F00.svg)](https://platformio.org)

**Library OTA (Over-The-Air) firmware update untuk ESP32 via HTTP/HTTPS.**

Library ringan untuk mengecek dan menerapkan update firmware dari **server HTTP/HTTPS manapun** — GitHub, VPS, S3, Firebase, server lokal LAN, atau URL apapun yang menyediakan file JSON manifest dan file `.bin`.

- **Auto Manifest Parsing**: Cukup berikan URL ke file JSON.
- **Cross-Version Support**: Mendukung ESP32 core 2.x dan 3.x secara otomatis.
- **Progress Callbacks**: Pantau progres download/flash secara real-time (Smart Filtering).
- **Update Notifications**: Dapatkan info versi sebelum download dimulai via `onUpdateAvailable`.
- **Lifecycle Callbacks**: Kontrol aksi pada event `onStart`, `onEnd`, dan `onError`.
- **Custom Headers**: Tambahkan Authorization atau meta-data ke request HTTP.
- **Retry Mechanism**: Lebih stabil dengan fitur auto-retry jika koneksi gagal.
- **Configurable Timeouts**: Kontrol perilaku user terhadap koneksi lambat.
- **Semantic Versioning**: Update hanya dilakukan jika versi di server lebih tinggi.
- **Network Agnostic**: Bekerja dengan `WiFiClient` (HTTP) atau `WiFiClientSecure` (HTTPS).
- **GitHub Ready**: Ambil update dari GitHub Releases atau raw content dengan mudah.

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

    // Cek dan menjalankan update (Standar)
    OTAResult result = ota.update(client);
    
    // Atau paksa update tanpa cek versi:
    // OTAResult result = ota.forceUpdate(client);
    
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
    OTAResult result = ota.update(client);

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

### `update(OTAClient& client)` → `OTAResult`

Mengecek manifest di server, membandingkan versi, dan update jika ada versi baru.

### `forceUpdate(OTAClient& client)` → `OTAResult`

Mengabaikan pengecekan versi dan langsung melakukan proses update firmware.

```cpp
// HTTPS
WiFiClientSecure secureClient;
secureClient.setInsecure();
OTAResult result = ota.update(secureClient); // Standar
OTAResult resultForce = ota.forceUpdate(secureClient); // Paksa

// HTTP
WiFiClient httpClient;
OTAResult result = ota.update(httpClient);
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
OTAResult result = ota.update(client);
Serial.println(ESP32httpOTA::resultToString(result));
// Output: "Success" / "Already up to date" / "HTTP request failed" / dll
```

---

## Contoh (Examples)

Library ini menyediakan beberapa contoh di folder `examples/`:
- **BasicOTA**: Cara standar melakukan update dengan pengecekan versi firmware.
- **ForceOTA**: Cara memaksa update langsung tanpa pengecekan versi.
- **IntegratedOTA (Project)**: Contoh integrasi lengkap dengan sensor (Ultrasonic, DHT, PIR), LED PWM, dan tampilan progres di OLED.


---

### `onProgress(OTAProgressCallback callback)`

Set function callback untuk memantau progres update.

```cpp
ota.onProgress([](int current, int total) {
    Serial.printf("Progres: %d%%\n", (current * 100) / total);
});
```

> **Note:** Library memiliki fitur **Smart Progress Filtering**. Callback ini hanya akan dipanggil jika persentase (%) berubah, sehingga tidak membanjiri Serial monitor atau memperlambat layar OLED.

---

### `onUpdateAvailable(OTAUpdateCallback callback)`

Callback yang dipanggil ketika versi baru ditemukan di server, sebelum proses download dimulai. Memberikan informasi versi saat ini dan versi terbaru.

```cpp
ota.onUpdateAvailable([](const String& current, const String& latest) {
    Serial.printf("Update ditemukan! v%s -> v%s\n", current.c_str(), latest.c_str());
});
```

---

### `setTimeout(uint32_t timeoutMs)`

Set timeout untuk request HTTP (default: 10000ms).

```cpp
ota.setTimeout(15000); // 15 detik
```

---

### `onStart(OTACallback callback)`
### `onEnd(OTACallback callback)`
### `onError(OTAErrorCallback callback)`

Lifecycle callbacks untuk menangani proses update.

```cpp
ota.onStart([]() { Serial.println("Update dimulai..."); });
ota.onEnd([]() { Serial.println("Update selesai sukses!"); });
ota.onError([](OTAResult err) { Serial.println("Gagal!"); });
```

---

### `addHeader(String name, String value)`

Menambahkan header kustom (misal: Auth).

```cpp
ota.addHeader("Authorization", "Bearer token123");
```

---

### `setRetries(int count)`

Mengatur jumlah percobaan ulang jika network error (default: 0).

```cpp
ota.setRetries(3);
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
