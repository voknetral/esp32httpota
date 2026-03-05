/**
 * ForceOTA.ino
 *
 * Update firmware langsung dari URL tertentu tanpa cek versi.
 * Berguna untuk recovery atau rollback ke versi spesifik.
 *
 * Contoh ini menggunakan HTTP biasa (bukan HTTPS) dengan WiFiClient.
 */

#define OTA_DEBUG
#include <ESP32httpOTA.h>
#include <WiFi.h>

const char *WIFI_SSID = "NamaWiFi";
const char *WIFI_PASSWORD = "PasswordWiFi";

// URL langsung ke file .bin (HTTP biasa)
const char *FIRMWARE_URL = "http://192.168.1.100/firmware.bin";

ESP32httpOTA ota("1.0.0", "http://192.168.1.100/version.json");

void setup() {
  Serial.begin(115200);
  Serial.printf("\n=== Firmware v%s ===\n", ota.currentVersion());

  // Progress callback
  ota.onProgress([](int current, int total) {
    if (total > 0) {
      int pct = (current * 100) / total;
      Serial.printf("[OTA] Progress: %d%%\n", pct);
    }
  });

  // Konek WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(" OK!");

  // Force update via HTTP — langsung download tanpa cek versi
  WiFiClient client;

  OTAResult result = ota.forceUpdate(client, FIRMWARE_URL);
  Serial.printf("Hasil: %s\n", ESP32httpOTA::resultToString(result));

  if (result == OTA_SUCCESS) {
    Serial.println("Update OK! Restarting...");
    delay(1000);
    ESP.restart();
  }
}

void loop() {
  // Tidak akan sampai sini jika update sukses (device restart di setup)
  Serial.println("Update gagal. Coba lagi dalam 30 detik...");
  delay(30000);
  ESP.restart();
}
