#include <WiFi.h>
#include <string.h>
#include <functional>
#include "WebInterface.h"
#include "AppConfig.h"
#include "WebPage.h"
#include "VehicleProfiles.h"
#include "ApiValidation.h"

namespace {
// Raw and multipart dispatch are distinct virtual methods in Core 3.3.11.
// Never share an upload callback or obtain raw data through server.raw().
class RawControlHandler final : public RequestHandler {
public:
  using BodyCallback = std::function<void(const HTTPRaw &)>;
  RawControlHandler(WebServer::THandlerFunction onSelect, WebServer::THandlerFunction onHandle, BodyCallback onBody)
    : selectCallback(onSelect), handleCallback(onHandle), bodyCallback(onBody) {}

  bool canHandle(HTTPMethod method, const String &uri) override {
    return method == HTTP_POST && uri == "/api/set";
  }
  bool canHandle(WebServer &, HTTPMethod method, const String &uri) override {
    if (!canHandle(method, uri)) return false;
    // Core calls this during request selection, before headers/body are read.
    // Reset even if multipart bypasses raw dispatch, so an old body cannot apply.
    selectCallback();
    return true;
  }
  bool canUpload(const String &) override { return false; }
  bool canUpload(WebServer &, const String &) override { return false; }
  bool canRaw(const String &uri) override { return uri == "/api/set"; }
  bool canRaw(WebServer &server, const String &uri) override {
    return canHandle(server.method(), uri);
  }
  bool handle(WebServer &, HTTPMethod method, const String &uri) override {
    if (!canHandle(method, uri)) return false;
    handleCallback();
    return true;
  }
  void raw(WebServer &server, const String &uri, HTTPRaw &event) override {
    if (canHandle(server.method(), uri)) bodyCallback(event);
  }
private:
  WebServer::THandlerFunction selectCallback;
  WebServer::THandlerFunction handleCallback;
  BodyCallback bodyCallback;
};
}

bool WebInterface::startAccessPoint() {
  WiFi.persistent(false);
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(250);
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  const bool configOk = WiFi.softAPConfig(Config::AP_IP, Config::AP_GATEWAY, Config::AP_MASK);
  const bool apOk = WiFi.softAP(Config::AP_SSID, Config::AP_PASSWORD, 6, false, 4);
  Serial.println();
  Serial.println("ESP32 Wi-Fi Stand Alone Test");
  Serial.printf("Firmware : v%s\n", Config::FW_VERSION);
  Serial.printf("AP config: %s\n", configOk ? "OK" : "FAILED");
  Serial.printf("AP start : %s\n", apOk ? "OK" : "FAILED");
  Serial.printf("SSID     : %s\n", Config::AP_SSID);
  Serial.printf("AP IP    : %s\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("AP MAC   : %s\n", WiFi.softAPmacAddress().c_str());
  return configOk && apOk;
}

void WebInterface::begin() {
  bool started = false;
  for (uint8_t attempt = 1; attempt <= 5 && !started; ++attempt) {
    Serial.printf("Starting Wi-Fi AP, attempt %u/5\n", attempt);
    started = startAccessPoint();
    if (!started) delay(750);
  }
  server.on("/", HTTP_GET, [this]() { server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate"); server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/api/state", HTTP_GET, [this]() { server.sendHeader("Cache-Control", "no-store"); server.send(200, "application/json", state.toJson()); });
  server.on("/api/profiles", HTTP_GET, [this]() { server.sendHeader("Cache-Control", "no-store"); server.send(200, "application/json", VehicleProfiles::toJson()); });
  const char *controlHeaders[] = {"Content-Type", "Content-Length", "Transfer-Encoding"};
  server.collectHeaders(controlHeaders, 3);
  server.addHandler(new RawControlHandler(
    [this]() { controlBodyLength = 0; controlBodyComplete = false; controlBodyOverflow = false; },
    [this]() { handleSet(); },
    [this](const HTTPRaw &event) { receiveControlBody(event); }));
  server.on("/api/set", HTTP_ANY, [this]() {
    server.sendHeader("Cache-Control", "no-store");
    server.sendHeader("Allow", "POST");
    server.send(405, "application/json", "{\"error\":\"use POST with text/plain command body\"}");
  });
  server.onNotFound([this]() { server.send(404, "application/json", "{\"error\":\"not found\"}"); });
  server.begin();
}

void WebInterface::handleClient() {
  server.handleClient();
  const uint32_t now = millis();
  if (now - lastHealthCheckMs < 5000) return;
  lastHealthCheckMs = now;
  if (WiFi.getMode() != WIFI_MODE_AP || (uint32_t)WiFi.softAPIP() == 0) {
    Serial.println("Wi-Fi AP health check failed; restarting AP...");
    startAccessPoint();
  }
}

void WebInterface::receiveControlBody(const HTTPRaw &raw) {
  static_assert(sizeof(controlBody) == ApiValidation::BodyLimit + 1, "Control buffer must match body cap");
  // This event is passed directly by Core's raw dispatch, never upload dispatch.
  if (raw.status == RAW_START) {
    controlBodyLength = 0;
    controlBodyComplete = false;
    controlBodyOverflow = false;
  } else if (raw.status == RAW_WRITE) {
    if (controlBodyOverflow || raw.currentSize > ApiValidation::BodyLimit - controlBodyLength) {
      controlBodyOverflow = true; // Discard subsequent chunks; no unbounded command buffer.
    } else {
      memcpy(controlBody + controlBodyLength, raw.buf, raw.currentSize);
      controlBodyLength += raw.currentSize;
    }
  } else if (raw.status == RAW_END) {
    controlBodyComplete = true;
  } else if (raw.status == RAW_ABORTED) {
    controlBodyComplete = false;
  }
}

void WebInterface::handleSet() {
  server.sendHeader("Cache-Control", "no-store");
  String contentType = server.header("Content-Type");
  const int semicolon = contentType.indexOf(';');
  if (semicolon >= 0) contentType = contentType.substring(0, semicolon);
  contentType.trim();
  if (contentType != "text/plain") {
    server.send(415, "application/json", "{\"error\":\"Content-Type must be text/plain\"}");
    return;
  }
  const String lengthHeader = server.header("Content-Length");
  uint32_t declaredLength = 0;
  if (server.hasHeader("Transfer-Encoding") ||
      lengthHeader.length() != strlen(lengthHeader.c_str()) ||
      !ApiValidation::parseUnsigned(lengthHeader.c_str(), ApiValidation::BodyLimit, declaredLength) ||
      declaredLength == 0 || server.clientContentLength() != int(declaredLength) ||
      !controlBodyComplete || controlBodyOverflow || controlBodyLength != declaredLength) {
    server.send(400, "application/json", "{\"error\":\"unsupported framing or body length\"}");
    return;
  }
  // Only raw body is authoritative. All POST query parameters are ignored.
  ApiValidation::Request request;
  const char *error = ApiValidation::parseBody(controlBody, controlBodyLength, Config::MAX_RPM, VehicleProfiles::count(), request);
  if (error) {
    server.send(400, "application/json", String("{\"error\":\"") + error + "\"}");
    return;
  }
  const auto &v = request.values;
  if (v[ApiValidation::Run] >= 0) state.running = v[ApiValidation::Run] == 1;
  if (v[ApiValidation::Rpm] >= 0) state.rpm = v[ApiValidation::Rpm];
  if (v[ApiValidation::Ckp] >= 0) state.ckpEnabled = v[ApiValidation::Ckp] == 1;
  if (v[ApiValidation::Cmp] >= 0) state.cmpEnabled = v[ApiValidation::Cmp] == 1;
  if (v[ApiValidation::Led] >= 0) state.ledTest = v[ApiValidation::Led] == 1;
  if (v[ApiValidation::Scope] >= 0) state.scopeTest = v[ApiValidation::Scope] == 1;
  if (v[ApiValidation::Profile] >= 0) state.vehicleProfile = v[ApiValidation::Profile];
  if (v[ApiValidation::SelfTest] == 1) state.startSelfTest();
  for (int field = ApiValidation::Tps; field <= ApiValidation::O2; ++field) {
    if (v[field] >= 0) state.setSensor(ApiValidation::names[field], v[field]);
  }
  signals.update(); // Publish the complete control snapshot before replying.
  server.send(200, "application/json", state.toJson());
}
