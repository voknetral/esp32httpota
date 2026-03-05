# ESP32httpOTA

[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)](https://github.com/voknetral/ESP32httpOTA)
[![Platform](https://img.shields.io/badge/platform-ESP32-green.svg)](https://github.com/espressif/arduino-esp32)
[![License](https://img.shields.io/badge/license-MIT-orange.svg)](LICENSE)
[![Arduino](https://img.shields.io/badge/Arduino_IDE-compatible-00979D.svg)](https://www.arduino.cc/)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-compatible-FF7F00.svg)](https://platformio.org)

**Library OTA (Over-The-Air) firmware update untuk ESP32 via HTTP/HTTPS.**

Library ringan dan modular untuk mengecek dan menerapkan update firmware dari **server HTTP/HTTPS manapun** — GitHub, VPS, S3, Firebase, server lokal LAN, atau URL apapun yang menyediakan file JSON manifest dan file `.bin`.

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
    client.setInsecure();          // atau ota.setCACert(rootCA) untuk verifikasi

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

### Konfigurasi

#### `setCACert(const char* cert)`

Set root CA certificate untuk verifikasi HTTPS. Jika tidak di-set, user harus pakai `client.setInsecure()`.

```cpp
const char* rootCA = "-----BEGIN CERTIFICATE-----\n...";
ota.setCACert(rootCA);
```

> **Note:** CA cert otomatis di-apply ke client saat `run()` atau `forceUpdate()` dipanggil.

---

#### `setManifestURL(const char* url)`

Ubah manifest URL saat runtime. Berguna jika URL bisa berubah (misalnya staging vs production).

```cpp
ota.setManifestURL("https://staging.server.com/version.json");
```

---

#### `setTimeout(uint32_t ms)`

Set timeout HTTP request dalam milidetik. Default: `10000` (10 detik).

```cpp
ota.setTimeout(15000);  // 15 detik
```

---

#### `rebootOnUpdate(bool reboot)`

Atur apakah device otomatis restart setelah update berhasil. Default: `false` (restart manual).

```cpp
ota.rebootOnUpdate(true);   // Auto restart setelah flash berhasil
ota.rebootOnUpdate(false);  // User yang panggil ESP.restart() sendiri
```

---

#### `onProgress(OTAProgressCallback cb)`

Register callback untuk monitoring progress download firmware.

**Signature callback:** `void(int current, int total)`

```cpp
ota.onProgress([](int current, int total) {
    if (total > 0) {
        int pct = (current * 100) / total;
        Serial.printf("Progress: %d%% (%d / %d bytes)\n", pct, current, total);
    }
});
```

> **Tip:** Jika `onProgress` tidak di-set dan `OTA_DEBUG` aktif, library otomatis print progress setiap 10%.

---

### OTA Operations

#### `run(WiFiClientSecure& client)` → `OTAResult`
#### `run(WiFiClient& client)` → `OTAResult`

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

> **Warning:** Library TIDAK restart device otomatis (kecuali `rebootOnUpdate(true)`). Cek `OTA_SUCCESS` dan panggil `ESP.restart()` sendiri.

---

#### `forceUpdate(WiFiClientSecure& client, const String& url)` → `OTAResult`
#### `forceUpdate(WiFiClient& client, const String& url)` → `OTAResult`

Download dan flash firmware langsung dari URL. **Tidak cek versi** — langsung download dan flash.

Berguna untuk:
- Recovery / rollback ke versi tertentu
- Force update tanpa manifest
- Testing firmware baru

```cpp
WiFiClient client;
OTAResult result = ota.forceUpdate(client, "http://192.168.1.100/firmware.bin");

if (result == OTA_SUCCESS) {
    ESP.restart();
}
```

---

### Getter

#### `currentVersion()` → `const char*`

Return versi firmware saat ini yang di-set di constructor.

```cpp
Serial.printf("Firmware v%s\n", ota.currentVersion());
// Output: Firmware v1.0.0
```

---

#### `latestVersion()` → `const char*`

Return versi terbaru yang ditemukan dari manifest **setelah** `run()` dipanggil. Kosong jika belum pernah cek.

```cpp
ota.run(client);
Serial.printf("Latest: v%s\n", ota.latestVersion());
// Output: Latest: v1.1.0
```

---

#### `resultToString(OTAResult result)` → `const char*` *(static)*

Konversi result code ke string yang mudah dibaca. Method static — bisa dipanggil tanpa instance.

```cpp
OTAResult result = ota.run(client);
Serial.println(ESP32httpOTA::resultToString(result));
// Output: "Success" / "Already up to date" / "HTTP request failed" / dll
```

---

## Result Codes

| Code | Nilai | Deskripsi | Kapan Terjadi |
|------|-------|-----------|---------------|
| `OTA_SUCCESS` | 0 | Update berhasil di-flash | Firmware baru sudah ditulis, siap restart |
| `OTA_NO_UPDATE` | 1 | Firmware sudah up to date | Versi di server ≤ versi saat ini |
| `OTA_HTTP_ERROR` | 2 | Gagal konek ke server | URL salah, server down, WiFi putus |
| `OTA_JSON_ERROR` | 3 | JSON invalid | Format salah, field `version`/`firmware` hilang |
| `OTA_UPDATE_FAILED` | 4 | Download/flash gagal | File `.bin` corrupt, koneksi putus saat download |

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

**Jika gagal:**

```
[OTA] HTTP error: -1
[OTA] Update failed (error -104): Connection refused
```

---

## Contoh Lengkap

### 1. BasicOTA — Cek Update via Tombol

Tekan tombol → konek WiFi → cek update → flash jika ada.

```cpp
#define OTA_DEBUG
#include <ESP32httpOTA.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

const char *WIFI_SSID = "NamaWiFi";
const char *WIFI_PASSWORD = "PasswordWiFi";
const int BUTTON_PIN = 12;

ESP32httpOTA ota("1.0.0",
    "https://raw.githubusercontent.com/username/repo/main/version.json");

bool connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return true;
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > 15000) return false;
    delay(500);
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  ota.onProgress([](int current, int total) {
    if (total > 0) {
      Serial.printf("[OTA] Progress: %d / %d bytes (%d%%)\n",
                    current, total, (current * 100) / total);
    }
  });

  Serial.printf("\n=== Firmware v%s ===\n", ota.currentVersion());
  Serial.println("Tekan tombol Pin 12 untuk cek OTA.");
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);
    if (digitalRead(BUTTON_PIN) == LOW) {
      if (connectWiFi()) {
        WiFiClientSecure client;
        client.setInsecure();
        OTAResult result = ota.run(client);
        Serial.printf("Hasil: %s\n", ESP32httpOTA::resultToString(result));
        if (result == OTA_SUCCESS) {
          delay(1000);
          ESP.restart();
        }
      }
      while (digitalRead(BUTTON_PIN) == LOW) delay(10);
    }
  }
}
```

---

### 2. AutoOTA — Cek Otomatis Berkala

Cek update otomatis setiap 1 jam. Cocok untuk perangkat IoT yang sudah terpasang.

```cpp
#define OTA_DEBUG
#include <ESP32httpOTA.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

const char *WIFI_SSID = "NamaWiFi";
const char *WIFI_PASSWORD = "PasswordWiFi";
const unsigned long CHECK_INTERVAL = 3600000; // 1 jam

ESP32httpOTA ota("1.0.0",
    "https://raw.githubusercontent.com/username/repo/main/version.json");

unsigned long lastCheck = 0;

void checkOTA() {
  WiFiClientSecure client;
  client.setInsecure();
  OTAResult result = ota.run(client);
  Serial.printf("OTA: %s\n", ESP32httpOTA::resultToString(result));
  if (result == OTA_SUCCESS) {
    delay(1000);
    ESP.restart();
  }
}

void setup() {
  Serial.begin(115200);
  ota.setTimeout(15000);
  ota.onProgress([](int cur, int total) {
    Serial.printf("[OTA] %d / %d bytes\n", cur, total);
  });

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  checkOTA();               // Cek saat boot
  lastCheck = millis();
}

void loop() {
  if (millis() - lastCheck >= CHECK_INTERVAL) {
    lastCheck = millis();
    if (WiFi.status() == WL_CONNECTED) checkOTA();
  }
  delay(1000);
}
```

---

### 3. ForceOTA — Flash Langsung via HTTP

Update firmware langsung dari URL tanpa cek versi. Menggunakan HTTP biasa.

```cpp
#define OTA_DEBUG
#include <ESP32httpOTA.h>
#include <WiFi.h>

const char *FIRMWARE_URL = "http://192.168.1.100/firmware.bin";
ESP32httpOTA ota("1.0.0", "http://192.168.1.100/version.json");

void setup() {
  Serial.begin(115200);

  ota.onProgress([](int current, int total) {
    if (total > 0) Serial.printf("[OTA] %d%%\n", (current * 100) / total);
  });

  WiFi.begin("SSID", "PASSWORD");
  while (WiFi.status() != WL_CONNECTED) delay(500);

  WiFiClient client;                              // HTTP biasa
  OTAResult result = ota.forceUpdate(client, FIRMWARE_URL);
  Serial.printf("Hasil: %s\n", ESP32httpOTA::resultToString(result));

  if (result == OTA_SUCCESS) {
    delay(1000);
    ESP.restart();
  }
}

void loop() {
  Serial.println("Update gagal. Retry 30 detik...");
  delay(30000);
  ESP.restart();
}
```

---

## Alur Kerja Update

```
+----------------------------------------------------+
|  DEVELOPER                                          |
|                                                     |
|  1. Edit kode, ubah versi ke "1.1.0"                |
|  2. Arduino IDE -> Sketch -> Export Compiled Binary  |
|  3. Upload firmware.bin ke server                   |
|  4. Update version.json -> { "version": "1.1.0" }  |
+------------------------+---------------------------+
                         |
                         v
+----------------------------------------------------+
|  SERVER (GitHub / VPS / S3 / LAN / Firebase)        |
|                                                     |
|  version.json -> { "version": "1.1.0",             |
|                    "firmware": "url/fw.bin" }        |
|  firmware.bin -> (binary baru)                      |
+------------------------+---------------------------+
                         |
                         v
+----------------------------------------------------+
|  ESP32 DEVICE (running v1.0.0)                      |
|                                                     |
|  ota.run(client)                                    |
|    -> GET version.json                              |
|    -> Parse: 1.1.0 > 1.0.0 -> update tersedia      |
|    -> Download firmware.bin                          |
|    -> Flash ke partisi OTA                          |
|    -> return OTA_SUCCESS                            |
|  ESP.restart() -> boot sebagai v1.1.0               |
+----------------------------------------------------+
```

---

## Arsitektur

```
+------------------------------------------+
|            Sketch kamu (.ino)            |
|                                          |
|  WiFi / Ethernet --> WiFiClient(Secure)  |
|                                          |
|  Trigger -----------------------+        |
|  (tombol / timer / perintah)    |        |
|                                 v        |
|                      +--------------+    |
|                      | ESP32httpOTA |    |
|                      |              |    |
|                      |  - Manifest  |    |
|                      |  - Version   |    |
|                      |  - Download  |    |
|                      |  - Flash     |    |
|                      +--------------+    |
|                                          |
|  if (OTA_SUCCESS) ESP.restart();         |
+------------------------------------------+
```

**Library bertanggung jawab untuk:** OTA logic (cek, download, flash).
**Kamu bertanggung jawab untuk:** Koneksi network, trigger update, restart, dan aplikasi.

---

## Struktur File

```
ESP32httpOTA/
├── src/
│   ├── ESP32httpOTA.h          # Header + Doxygen documentation
│   └── ESP32httpOTA.cpp         # Implementasi
├── examples/
│   ├── BasicOTA/
│   │   └── BasicOTA.ino        # Cek update via tombol (HTTPS)
│   ├── AutoOTA/
│   │   └── AutoOTA.ino         # Cek otomatis berkala (HTTPS)
│   └── ForceOTA/
│       └── ForceOTA.ino        # Flash langsung via URL (HTTP)
├── library.properties          # Metadata Arduino IDE
├── library.json                # Metadata PlatformIO
├── keywords.txt                # Syntax highlighting Arduino IDE
├── LICENSE                     # MIT License
└── README.md                   # Dokumentasi ini
```

---

## FAQ

### Apakah library ini support ESP8266?
Tidak. Library ini khusus untuk **ESP32** karena menggunakan `HTTPUpdate` dari arduino-esp32 core.

### Harus pakai HTTPS?
Tidak. Library support HTTP biasa dengan `WiFiClient`. Tapi untuk production disarankan HTTPS.

### Bisa pakai Ethernet selain WiFi?
Bisa. Selama kamu punya `Client` yang terkoneksi, library bisa dipakai. `run()` menerima `WiFiClient&` yang merupakan turunan dari `Client`.

### `OTA_DEBUG` akan memperbesar binary?
Jika tidak di-define, semua `OTA_LOG()` akan hilang total saat compile (macro kosong). Tidak ada overhead.

### Apakah aman jika update gagal di tengah jalan?
Ya. ESP32 menggunakan dual-partition scheme. Jika flash gagal, device tetap boot dari partisi yang lama.

### Kenapa library tidak restart sendiri?
Best practice: library tidak boleh mengambil kontrol restart. User mungkin perlu menyimpan data, menutup koneksi, atau memberi notifikasi sebelum restart.

---

## Changelog

### v1.0.0
- Initial release
- JSON manifest-based version checking
- Semantic version comparison
- HTTP dan HTTPS support (`WiFiClient` & `WiFiClientSecure`)
- CA certificate support dengan `setCACert()`
- Progress callback dengan `onProgress()`
- Configurable timeout dengan `setTimeout()`
- Auto-reboot control dengan `rebootOnUpdate()`
- Debug logging dengan `#define OTA_DEBUG`
- Force update dari direct URL
- Arduino IDE + PlatformIO compatible

---

## License

MIT License — lihat [LICENSE](LICENSE) untuk detail.

---

**Made by [voknetral](https://github.com/voknetral)**
