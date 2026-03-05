/**
 * AutoOTA.ino
 *
 * Cek update otomatis setiap interval waktu tertentu.
 * Tidak perlu tombol — cocok untuk perangkat IoT yang sudah terpasang.
 */

#define OTA_DEBUG
#include <ESP32httpOTA.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// ── Konfigurasi ──
const char *WIFI_SSID = "NamaWiFi";
const char *WIFI_PASSWORD = "PasswordWiFi";

// Cek update setiap 1 jam (3600000 ms)
const unsigned long CHECK_INTERVAL = 3600000;

ESP32httpOTA
    ota("1.0.0",
        "https://raw.githubusercontent.com/username/repo/main/version.json");

unsigned long lastCheck = 0;

// ── Helper: Cek dan apply update ──
void checkOTA() {
  WiFiClientSecure client;
  client.setInsecure();

  OTAResult result = ota.run(client);
  Serial.printf("OTA: %s\n", ESP32httpOTA::resultToString(result));

  if (result == OTA_SUCCESS) {
    Serial.println("Update OK! Restarting...");
    delay(1000);
    ESP.restart();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.printf("\n=== Firmware v%s ===\n", ota.currentVersion());

  // Konfigurasi timeout (opsional)
  ota.setTimeout(15000);

  // Progress callback (opsional)
  ota.onProgress([](int current, int total) {
    Serial.printf("[OTA] %d / %d bytes\n", current, total);
  });

  // Konek WiFi sekali di awal
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.printf(" OK! (%s)\n\n", WiFi.localIP().toString().c_str());

  // Cek update langsung saat boot
  checkOTA();
  lastCheck = millis();
}

void loop() {
  // Cek update berkala
  if (millis() - lastCheck >= CHECK_INTERVAL) {
    lastCheck = millis();

    if (WiFi.status() == WL_CONNECTED) {
      checkOTA();
    }
  }

  // --- Program utamamu di sini ---
  delay(1000);
}
