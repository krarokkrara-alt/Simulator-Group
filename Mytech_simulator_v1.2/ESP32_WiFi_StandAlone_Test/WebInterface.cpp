#include <WiFi.h>
#include "WebInterface.h"
#include "AppConfig.h"
#include "WebPage.h"
#include "VehicleProfiles.h"

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
  server.on("/api/set", HTTP_GET, [this]() { handleSet(); });
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

void WebInterface::handleSet() {
  if (server.hasArg("run")) state.running = server.arg("run") == "1";
  if (server.hasArg("rpm")) state.rpm = constrain(server.arg("rpm").toInt(), 0, Config::MAX_RPM);
  if (server.hasArg("ckp")) state.ckpEnabled = server.arg("ckp") == "1";
  if (server.hasArg("cmp")) state.cmpEnabled = server.arg("cmp") == "1";
  if (server.hasArg("led")) state.ledTest = server.arg("led") == "1";
  if (server.hasArg("scope")) state.scopeTest = server.arg("scope") == "1";
  if (server.hasArg("profile")) state.vehicleProfile = constrain(server.arg("profile").toInt(), 0, (int)VehicleProfiles::count() - 1);
  if (server.hasArg("selftest") && server.arg("selftest") == "1") state.startSelfTest();
  for (const char *name : {"tps", "map", "ect", "iat", "o2"}) if (server.hasArg(name)) state.setSensor(name, server.arg(name).toInt());
  server.send(200, "application/json", state.toJson());
}
