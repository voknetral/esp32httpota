/**
 * BasicOTA.ino
 *
 * Contoh dasar ESP32httpOTA: cek update saat tombol ditekan.
 * Tekan tombol di Pin 12 → konek WiFi → cek server → update jika ada.
 */

#define OTA_DEBUG // Aktifkan log debug di Serial Monitor
#include <ESP32httpOTA.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// ── Konfigurasi ──
const char *WIFI_SSID = "NamaWiFi";
const char *WIFI_PASSWORD = "PasswordWiFi";
const int BUTTON_PIN = 12;

ESP32httpOTA
    ota("1.0.0",
        "https://raw.githubusercontent.com/username/repo/main/version.json");

// ── Helper: Konek WiFi ──
bool connectWiFi() {
  if (WiFi.status() == WL_CONNECTED)
    return true;
  Serial.print("WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > 15000) {
      Serial.println(" GAGAL!");
      return false;
    }
    Serial.print(".");
    delay(500);
  }
  Serial.printf(" OK! (%s)\n", WiFi.localIP().toString().c_str());
  return true;
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Progress callback (opsional)
  ota.onProgress([](int current, int total) {
    if (total > 0) {
      Serial.printf("[OTA] Progress: %d / %d bytes (%d%%)\n", current, total,
                    (current * 100) / total);
    }
  });

  Serial.printf("\n=== Firmware v%s ===\n", ota.currentVersion());
  Serial.println("Tekan tombol Pin 12 untuk cek OTA.\n");
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);
    if (digitalRead(BUTTON_PIN) == LOW) {
      if (connectWiFi()) {
        WiFiClientSecure client;
        client.setInsecure();

        OTAResult result = ota.run(client);
        Serial.printf("Hasil: %s\n\n", ESP32httpOTA::resultToString(result));

        // Restart manual jika update sukses
        if (result == OTA_SUCCESS) {
          Serial.println("Restarting...");
          delay(1000);
          ESP.restart();
        }
      }
      while (digitalRead(BUTTON_PIN) == LOW)
        delay(10);
    }
  }
}
