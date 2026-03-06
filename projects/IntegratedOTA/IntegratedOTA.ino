#define OTA_DEBUG
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <ESP32httpOTA.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>

/**
 * PROJECT: Integrated IoT Node with OTA
 *
 * Features:
 * - 1x Ultrasonic Sensor (HC-SR04)
 * - DHT22 Temperature & Humidity
 * - PIR Motion Detection
 * - 1x Dimming LED (PWM)
 * - OLED Display Menu System
 * - Professional OTA Update (GitHub Hosted)
 */

// ================= WIFI =================
const char *ssid = "TEKNOLAB Office";
const char *password = "selamatdatang";

// ================= OTA =================
const char *current_version = "1.1.1";
const char *manifest_url = "https://raw.githubusercontent.com/voknetral/"
                           "Test-OTA-Firmware/refs/heads/main/version.json";

ESP32httpOTA ota(current_version, manifest_url);

// ================= OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define TRIG1 17
#define ECHO1 16

#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define PIR_PIN 35

#define LED1 18

#define BTN_NEXT 25
#define BTN_PREV 27

#define PIN_D5 5

// ================= PWM SETTINGS =================
#define PWM_FREQ 5000
#define PWM_RES 8

// ================= GLOBAL VARIABLES =================
float dist1;
float temperature, humidity;
bool motion;
int menuIndex = 0;

// ================= HELPER: READ ULTRASONIC =================
float readDistance(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long duration = pulseIn(echo, HIGH, 25000); // Timeout 25ms
  float dist = duration * 0.034 / 2;

  if (dist <= 0 || dist > 400)
    return 400.0;
  return dist;
}

// ================= UI: SHOW OTA PROGRESS =================
void showOTAUI(const char *status, int progress) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("FIRMWARE UPDATE");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  display.setCursor(0, 20);
  display.println(status);

  if (progress >= 0) {
    // Progress Bar
    display.drawRect(5, 35, 118, 12, SSD1306_WHITE);
    int barWidth = map(progress, 0, 100, 0, 114);
    display.fillRect(7, 37, barWidth, 8, SSD1306_WHITE);

    display.setCursor(45, 52);
    display.print(progress);
    display.print("%");
  }

  display.display();
}

// ================= LOGIC: UPDATE OLED (MENU) =================
void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("v");
  display.println(ota.currentVersion());
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  display.setCursor(0, 15);

  switch (menuIndex) {
  case 0:
    display.println("> DISTANCE");
    display.setTextSize(2);
    display.printf("%.1f cm", dist1);
    break;
  case 1:
    display.println("> ENV SENSOR");
    display.printf("Temp: %.1f C\n", isnan(temperature) ? 0 : temperature);
    display.printf("Hum : %.1f %%\n", isnan(humidity) ? 0 : humidity);
    break;
  case 2:
    display.println("> MOTION");
    display.setTextSize(2);
    display.println(motion ? "DETECTED" : "CLEAR");
    break;
  }
  display.display();
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  // Pin Modes
  pinMode(PIN_D5, OUTPUT);
  pinMode(TRIG1, OUTPUT);
  pinMode(ECHO1, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);

  // LED PWM Setup (ESP32 Core 3.x style)
  ledcAttach(LED1, PWM_FREQ, PWM_RES);

  dht.begin();
  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED Failed!");
    while (1)
      ;
  }
  display.setTextColor(SSD1306_WHITE);
  showOTAUI("Connecting WiFi...", -1);

  // Connect WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");

  // ================= OTA LIFECYCLE CALLBACKS =================

  ota.onUpdateAvailable([](const String &current, const String &latest) {
    Serial.printf("\n[!] Update tersedia! v%s -> v%s\n", current.c_str(),
                  latest.c_str());
  });

  ota.onStart([]() {
    Serial.println("OTA: Start");
    showOTAUI("Downloading...", 0);
  });

  ota.onProgress([](int current, int total) {
    int pct = (current * 100) / total;
    showOTAUI("Downloading...", pct);
    Serial.printf("OTA: %d%%   \r", pct);
    if (pct == 100)
      Serial.println();
  });

  ota.onEnd([]() {
    showOTAUI("Internalizing...", 100);
    Serial.println("\nOTA: Success");
  });

  ota.onError([](OTAResult err) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Error: %s", ESP32httpOTA::resultToString(err));
    showOTAUI(buf, -1);
    delay(3000);
  });

  // Execute Update with separate client
  WiFiClientSecure client;
  client.setInsecure();

  showOTAUI("Checking Version...", -1);
  OTAResult res = ota.update(client);

  if (res == OTA_SUCCESS) {
    showOTAUI("Success! Rebooting", -1);
    delay(2000);
    ESP.restart();
  }

  // If we reach here, no update was done
  updateOLED();
}

// ================= LOOP =================
void loop() {
  // Navigation
  if (digitalRead(BTN_NEXT) == LOW) {
    menuIndex = (menuIndex + 1) % 3;
    delay(200);
  }
  if (digitalRead(BTN_PREV) == LOW) {
    menuIndex = (menuIndex - 1 + 3) % 3;
    delay(200);
  }

  // Read Sensors
  dist1 = readDistance(TRIG1, ECHO1);
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  motion = digitalRead(PIR_PIN);

  // LED Intensity based on Distance (Brightest when close, Dimmer when far)
  ledcWrite(LED1, constrain(map(dist1, 100, 5, 0, 255), 0, 255));

  updateOLED();

  // Debug Serial
  Serial.printf("Dist:%.1f T:%.1f H:%.1f M:%d\n", dist1, temperature, humidity,
                motion);

  delay(100);
}
