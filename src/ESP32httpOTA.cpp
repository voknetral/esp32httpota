/**
 * @file ESP32httpOTA.cpp
 * @brief Implementation of ESP32httpOTA library
 */

#include "ESP32httpOTA.h"

// ─── Constructor & Setters ──────────────────────────────────────────

ESP32httpOTA::ESP32httpOTA(const char *version, const char *manifestUrl)
    : _version(version), _manifestUrl(manifestUrl), _caCert(nullptr),
      _timeout(10000), _reboot(false), _progressCb(nullptr) {}

void ESP32httpOTA::setCACert(const char *cert) { _caCert = cert; }

void ESP32httpOTA::setManifestURL(const char *url) { _manifestUrl = url; }

void ESP32httpOTA::setTimeout(uint32_t ms) { _timeout = ms; }

void ESP32httpOTA::rebootOnUpdate(bool reboot) { _reboot = reboot; }

void ESP32httpOTA::onProgress(OTAProgressCallback cb) { _progressCb = cb; }

const char *ESP32httpOTA::currentVersion() { return _version.c_str(); }

const char *ESP32httpOTA::latestVersion() { return _latestVersion.c_str(); }

const char *ESP32httpOTA::resultToString(OTAResult result) {
  switch (result) {
  case OTA_SUCCESS:
    return "Success";
  case OTA_NO_UPDATE:
    return "Already up to date";
  case OTA_HTTP_ERROR:
    return "HTTP request failed";
  case OTA_JSON_ERROR:
    return "Invalid JSON manifest";
  case OTA_UPDATE_FAILED:
    return "Firmware update failed";
  default:
    return "Unknown";
  }
}

// ─── Semantic Version Compare ───────────────────────────────────────
// Returns: 1 (v1 > v2), 0 (equal), -1 (v1 < v2)

int ESP32httpOTA::_compareVersion(const String &v1, const String &v2) {
  int p1 = 0, p2 = 0;
  while (p1 < (int)v1.length() || p2 < (int)v2.length()) {
    int n1 = 0, n2 = 0;
    while (p1 < (int)v1.length() && v1[p1] != '.')
      n1 = n1 * 10 + (v1[p1++] - '0');
    while (p2 < (int)v2.length() && v2[p2] != '.')
      n2 = n2 * 10 + (v2[p2++] - '0');
    if (n1 != n2)
      return (n1 > n2) ? 1 : -1;
    p1++;
    p2++;
  }
  return 0;
}

// ─── Public run() overloads ─────────────────────────────────────────

OTAResult ESP32httpOTA::run(WiFiClientSecure &client) {
  // Apply CA certificate if set
  if (_caCert) {
    client.setCACert(_caCert);
  }
  return _fetchAndUpdate(client);
}

OTAResult ESP32httpOTA::run(WiFiClient &client) {
  return _fetchAndUpdate(client);
}

// ─── Public forceUpdate() overloads ─────────────────────────────────

OTAResult ESP32httpOTA::forceUpdate(WiFiClientSecure &client,
                                    const String &url) {
  if (_caCert) {
    client.setCACert(_caCert);
  }
  OTA_LOG("Force update from: %s", url.c_str());
  return _doUpdate(client, url);
}

OTAResult ESP32httpOTA::forceUpdate(WiFiClient &client, const String &url) {
  OTA_LOG("Force update from: %s", url.c_str());
  return _doUpdate(client, url);
}

// ─── Main OTA Flow ──────────────────────────────────────────────────

OTAResult ESP32httpOTA::_fetchAndUpdate(Client &client) {
  OTA_LOG("Checking manifest: %s", _manifestUrl.c_str());

  // 1. Fetch manifest
  HTTPClient http;
  if (!http.begin(client, _manifestUrl)) {
    OTA_LOG("http.begin() failed");
    return OTA_HTTP_ERROR;
  }

  http.setTimeout(_timeout);
  int code = http.GET();

  if (code != HTTP_CODE_OK) {
    OTA_LOG("HTTP error: %d", code);
    http.end();
    return OTA_HTTP_ERROR;
  }

  String payload = http.getString();
  http.end();
  OTA_LOG("Manifest received (%d bytes)", payload.length());

  // 2. Parse JSON (ArduinoJson v7)
  JsonDocument doc;
  if (deserializeJson(doc, payload)) {
    OTA_LOG("JSON parse error");
    return OTA_JSON_ERROR;
  }

  if (!doc.containsKey("version") || !doc.containsKey("firmware")) {
    OTA_LOG("JSON missing 'version' or 'firmware' field");
    return OTA_JSON_ERROR;
  }

  _latestVersion = doc["version"].as<String>();
  String firmwareUrl = doc["firmware"].as<String>();

  OTA_LOG("Current: v%s | Latest: v%s", _version.c_str(),
          _latestVersion.c_str());

  // 3. Compare versions
  if (_compareVersion(_latestVersion, _version) <= 0) {
    OTA_LOG("Already up to date");
    return OTA_NO_UPDATE;
  }

  OTA_LOG("Update available: v%s -> v%s", _version.c_str(),
          _latestVersion.c_str());

  // 4. Download & flash
  return _doUpdate(client, firmwareUrl);
}

// ─── Firmware Download & Flash ──────────────────────────────────────

OTAResult ESP32httpOTA::_doUpdate(Client &client, const String &url) {
  OTA_LOG("Downloading: %s", url.c_str());

  httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  httpUpdate.rebootOnUpdate(_reboot);

  // User progress callback
  if (_progressCb) {
    OTAProgressCallback cb = _progressCb; // capture by value
    httpUpdate.onProgress([cb](int current, int total) {
      if (total > 0) {
        cb(current, total);
      }
    });
  }
#ifdef OTA_DEBUG
  else {
    // Default debug progress when no user callback is set
    httpUpdate.onProgress([](int current, int total) {
      if (total > 0) {
        int pct = (current * 100) / total;
        if (pct % 10 == 0) {
          Serial.printf("[OTA] Progress: %d%%\n", pct);
        }
      }
    });
  }
#endif

  t_httpUpdate_return ret = httpUpdate.update(client, url);

  if (ret == HTTP_UPDATE_OK) {
    OTA_LOG("Update successful!");
    return OTA_SUCCESS;
  }

  OTA_LOG("Update failed (error %d): %s", httpUpdate.getLastError(),
          httpUpdate.getLastErrorString().c_str());
  return OTA_UPDATE_FAILED;
}
