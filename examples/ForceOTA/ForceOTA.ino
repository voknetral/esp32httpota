#define OTA_DEBUG
#include <ESP32httpOTA.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>


// WiFi Configuration
const char *ssid = "YOUR_SSID";
const char *password = "YOUR_PASSWORD";

// OTA Configuration
// In ForceOTA, it doesn't matter what current_version is.
const char *current_version = "1.0.0";
const char *manifest_url = "https://raw.githubusercontent.com/voknetral/"
                           "ESP32httpOTA/main/examples/BasicOTA/version.json";

// Create OTA instance
ESP32httpOTA ota(current_version, manifest_url);

void setup() {
  Serial.begin(115200);
  Serial.println("\n--- ESP32 Force OTA ---");
  Serial.println("Warning: This will flash firmware regardless of version!\n");

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  // Progress UI
  ota.onProgress([](int current, int total) {
    Serial.printf("Progress: %d%%\r", (current * 100) / total);
    if (current == total)
      Serial.println();
  });

  Serial.println("Forcing update now...");

  WiFiClientSecure client;
  client.setInsecure();

  // forceUpdate() skips version check
  OTAResult result = ota.forceUpdate(client);

  if (result == OTA_SUCCESS) {
    Serial.println("Force update success! Restarting...");
    delay(2000);
    ESP.restart();
  } else {
    Serial.printf("OTA Status: %s\n", ESP32httpOTA::resultToString(result));
  }
}

void loop() {}
