#include "SimulatorState.h"

void SimulatorState::begin() {
  pinMode(Config::LED_PIN, OUTPUT);
  pinMode(Config::SCOPE_PIN, OUTPUT);
  for (uint8_t pin : {Config::TPS_PIN, Config::MAP_PIN, Config::ECT_PIN, Config::IAT_PIN, Config::O2_PIN}) {
    pinMode(pin, OUTPUT);
    analogWrite(pin, 0);
  }
  setSensor("tps", sensors.tps);
  setSensor("map", sensors.map);
  setSensor("ect", sensors.ect);
  setSensor("iat", sensors.iat);
  setSensor("o2", sensors.o2);
}

void SimulatorState::setSensor(const String &name, int value) {
  const uint8_t v = constrain(value, 0, 100);
  uint8_t pin = 255;
  if (name == "tps") { sensors.tps = v; pin = Config::TPS_PIN; }
  else if (name == "map") { sensors.map = v; pin = Config::MAP_PIN; }
  else if (name == "ect") { sensors.ect = v; pin = Config::ECT_PIN; }
  else if (name == "iat") { sensors.iat = v; pin = Config::IAT_PIN; }
  else if (name == "o2") { sensors.o2 = v; pin = Config::O2_PIN; }
  if (pin != 255) analogWrite(pin, map(v, 0, 100, 0, 255));
}

void SimulatorState::startSelfTest() {
  selfTestActive = true;
  selfTestStartedAt = millis();
  ledTest = scopeTest = true;
  digitalWrite(Config::LED_PIN, HIGH);
}

void SimulatorState::updateSelfTest() {
  if (!selfTestActive) return;
  const uint32_t elapsed = millis() - selfTestStartedAt;
  digitalWrite(Config::LED_PIN, (elapsed / 200) % 2);
  if (elapsed >= 3000) {
    selfTestActive = ledTest = scopeTest = false;
    digitalWrite(Config::LED_PIN, LOW);
    digitalWrite(Config::SCOPE_PIN, LOW);
  }
}

String SimulatorState::toJson() const {
  String s = "{\"version\":\"" + String(Config::FW_VERSION) + "\"";
  s += ",\"running\":" + String(running ? "true" : "false");
  s += ",\"rpm\":" + String(rpm);
  s += ",\"ckp\":" + String(ckpEnabled ? "true" : "false");
  s += ",\"cmp\":" + String(cmpEnabled ? "true" : "false");
  s += ",\"led\":" + String(ledTest ? "true" : "false");
  s += ",\"scope\":" + String(scopeTest ? "true" : "false");
  s += ",\"selfTest\":" + String(selfTestActive ? "true" : "false");
  s += ",\"profile\":" + String(vehicleProfile);
  s += ",\"tps\":" + String(sensors.tps) + ",\"map\":" + String(sensors.map);
  s += ",\"ect\":" + String(sensors.ect) + ",\"iat\":" + String(sensors.iat);
  s += ",\"o2\":" + String(sensors.o2) + "}";
  return s;
}
