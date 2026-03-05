/**
 * @file ESP32httpOTA.h
 * @brief OTA Firmware Update Engine for ESP32
 * @version 1.0.0
 * @license MIT
 *
 * Lightweight library for performing Over-The-Air firmware updates
 * on ESP32 devices via HTTP/HTTPS. Works with any server — GitHub, VPS,
 * S3, self-hosted, or any URL serving a JSON manifest and .bin file.
 *
 * Requires ArduinoJson v7+
 */

#ifndef ESP32HTTPOTA_H
#define ESP32HTTPOTA_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

/** Library version */
#define ESP32HTTPOTA_VERSION "1.0.0"

/**
 * @brief Enable/disable debug serial output.
 * Define OTA_DEBUG before including this header to enable.
 * @code
 * #define OTA_DEBUG    // Enable debug output
 * #include <ESP32httpOTA.h>
 * @endcode
 */
#ifdef OTA_DEBUG
#define OTA_LOG(fmt, ...) Serial.printf("[OTA] " fmt "\n", ##__VA_ARGS__)
#else
#define OTA_LOG(fmt, ...)
#endif

/**
 * @enum OTAResult
 * @brief Result codes returned by OTA operations.
 */
enum OTAResult {
  OTA_SUCCESS = 0,  ///< Update flashed successfully (device NOT restarted)
  OTA_NO_UPDATE,    ///< Firmware is already up to date
  OTA_HTTP_ERROR,   ///< Failed to reach manifest URL
  OTA_JSON_ERROR,   ///< Manifest JSON is invalid or missing fields
  OTA_UPDATE_FAILED ///< Firmware download or flash failed
};

/**
 * @class ESP32httpOTA
 * @brief OTA update engine that checks and applies firmware updates from
 * any HTTP/HTTPS server.
 *
 * Usage (HTTPS):
 * @code
 * ESP32httpOTA ota("1.0.0",
 *   "https://raw.githubusercontent.com/user/repo/main/version.json");
 *
 * WiFiClientSecure client;
 * client.setInsecure();
 *
 * OTAResult result = ota.run(client);
 * if (result == OTA_SUCCESS) ESP.restart();
 * @endcode
 *
 * Usage (HTTP):
 * @code
 * ESP32httpOTA ota("1.0.0", "http://192.168.1.100/version.json");
 *
 * WiFiClient client;
 * OTAResult result = ota.run(client);
 * if (result == OTA_SUCCESS) ESP.restart();
 * @endcode
 */
class ESP32httpOTA {
public:
  /**
   * @brief Construct a new ESP32httpOTA instance.
   * @param version       Current firmware version string (e.g. "1.0.0")
   * @param manifestUrl   URL to version.json manifest
   */
  ESP32httpOTA(const char *version, const char *manifestUrl);

  // ─── OTA Operations ───────────────────────────────────────────────

  /**
   * @brief Check for update and apply if available (HTTPS).
   *
   * Steps performed:
   * 1. GET version.json from server
   * 2. Parse latest version & firmware URL
   * 3. Compare versions using Semantic Versioning
   * 4. If newer version exists: download & flash
   *
   * @param client  WiFiClientSecure reference (configured by caller)
   * @return OTAResult indicating the outcome
   */
  OTAResult run(WiFiClientSecure &client);

  /**
   * @brief Check for update and apply if available (HTTP).
   * @param client  WiFiClient reference
   * @return OTAResult indicating the outcome
   */
  OTAResult run(WiFiClient &client);

  // ─── Getters ──────────────────────────────────────────────────────

  /**
   * @brief Get current firmware version string.
   * @return Pointer to version string (e.g. "1.0.0")
   */
  const char *currentVersion();

  /**
   * @brief Convert OTAResult code to human-readable string.
   * @param result OTAResult code
   * @return Descriptive string
   */
  static const char *resultToString(OTAResult result);

private:
  String _version;
  String _manifestUrl;

  int _compareVersion(const String &v1, const String &v2);
  OTAResult _fetchAndUpdate(Client &client);
  OTAResult _doUpdate(Client &client, const String &url);
};

#endif // ESP32HTTPOTA_H
