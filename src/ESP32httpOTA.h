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
#include <NetworkClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <map>
#include <vector>

#ifdef ESP_ARDUINO_VERSION
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
#include <NetworkClient.h>
typedef NetworkClient OTAClient;
#else
typedef Client OTAClient;
#endif
#else
typedef Client OTAClient;
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

typedef std::function<void(int, int)> OTAProgressCallback;
typedef std::function<void()> OTACallback;
typedef std::function<void(OTAResult)> OTAErrorCallback;

struct OTAHeader {
  String name;
  String value;
};

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
   * @brief Check for update and apply if available.
   * @param client  WiFiClientSecure/WiFiClient reference
   * @return OTAResult indicating the outcome
   */
  OTAResult update(WiFiClientSecure &client);
  OTAResult update(WiFiClient &client);

  /**
   * @brief Skip version check and force firmware update.
   * @param client  WiFiClientSecure/WiFiClient reference
   * @return OTAResult indicating the outcome
   */
  OTAResult forceUpdate(WiFiClientSecure &client);
  OTAResult forceUpdate(WiFiClient &client);

  /** @deprecated Use update() instead */
  OTAResult run(WiFiClientSecure &client) { return update(client); }
  /** @deprecated Use update() instead */
  OTAResult run(WiFiClient &client) { return update(client); }

  // ─── Configuration ──────────────────────────────────────────────

  /**
   * @brief Set progress callback function.
   * @param callback Function with signature: void(int current, int total)
   */
  void onProgress(OTAProgressCallback callback);

  /**
   * @brief Set HTTP request timeout.
   * @param timeoutMs Timeout in milliseconds (default: 10000)
   */
  void setTimeout(uint32_t timeoutMs);

  /**
   * @brief Set callback for when update starts.
   */
  void onStart(OTACallback callback);

  /**
   * @brief Set callback for when update ends successfully.
   */
  void onEnd(OTACallback callback);

  /**
   * @brief Set callback for when an error occurs.
   */
  void onError(OTAErrorCallback callback);

  /**
   * @brief Add a custom HTTP header to all requests.
   * @param name   Header name (e.g. "Authorization")
   * @param value  Header value (e.g. "Bearer token")
   */
  void addHeader(const String &name, const String &value);

  /**
   * @brief Clear all custom HTTP headers.
   */
  void clearHeaders();

  /**
   * @brief Set number of retries for failed requests.
   * @param count  Number of retries (default: 0)
   */
  void setRetries(int count);

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

  OTAResult _fetchAndUpdate(OTAClient &client, bool force = false);
  OTAResult _doUpdate(OTAClient &client, const String &url);

  OTAProgressCallback _progressCb;
  OTACallback _startCb;
  OTACallback _endCb;
  OTAErrorCallback _errorCb;

  std::vector<OTAHeader> _headers;
  uint32_t _timeout = 10000;
  int _retries = 0;

  void _applyHeaders(HTTPClient &http);
};

#endif // ESP32HTTPOTA_H
