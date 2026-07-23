#include "board_health.h"
#include "config.h"
#include <esp_system.h>

void BoardHealthMonitor::begin() {
  analogReadResolution(12);
  analogSetPinAttenuation(Config::ADC_MAP_FEEDBACK_PIN, ADC_11db);
  analogSetPinAttenuation(Config::ADC_TPS_FEEDBACK_PIN, ADC_11db);
}

float BoardHealthMonitor::readVoltage(uint8_t pin) {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < 32; ++i) {
    sum += analogRead(pin);
    delayMicroseconds(150);
  }
  return (sum / 32.0f) * Config::ADC_FULL_SCALE_V / 4095.0f;
}

HealthLevel BoardHealthMonitor::compareVoltage(float commanded, float measured) const {
  const float error = fabsf(commanded - measured);
  if (error <= Config::FEEDBACK_TOLERANCE_V) return HealthLevel::PASS;
  if (measured < 0.05f) return HealthLevel::NOT_TESTED;  // likely no feedback jumper
  if (error <= Config::FEEDBACK_TOLERANCE_V * 2.0f) return HealthLevel::WARN;
  return HealthLevel::FAIL;
}

BoardHealth BoardHealthMonitor::runFullTest(
    SignalEngine& signals, SimulatorState& state, bool wifiEnabled) {
  const uint32_t started = millis();
  BoardHealth result;
  static uint32_t totalTests = 0;
  result.testCount = ++totalTests;

  const bool wasRunning = state.running;
  const uint16_t previousRpm = state.rpm;
  state.running = false;
  signals.allOff();

  result.mapCommandV = 1.00f;
  result.tpsCommandV = 2.00f;
  signals.setAnalog(result.mapCommandV, result.tpsCommandV);
  delay(25);
  result.mapMeasuredV = readVoltage(Config::ADC_MAP_FEEDBACK_PIN);
  result.tpsMeasuredV = readVoltage(Config::ADC_TPS_FEEDBACK_PIN);
  result.adcMap = compareVoltage(result.mapCommandV, result.mapMeasuredV);
  result.adcTps = compareVoltage(result.tpsCommandV, result.tpsMeasuredV);

  result.freeHeapBytes = ESP.getFreeHeap();
  result.memory = result.freeHeapBytes >= Config::MIN_HEAP_WARN_BYTES
                      ? HealthLevel::PASS : HealthLevel::WARN;

  signals.clearJitter();
  state.rpm = 1000;
  state.running = true;
  const uint32_t timingStarted = millis();
  while (millis() - timingStarted < 300) {
    signals.update(state);
    delayMicroseconds(20);
  }
  state.running = false;
  signals.allOff();
  result.maxLoopJitterUs = signals.maxJitterUs();
  result.timing = result.maxLoopJitterUs <= Config::JITTER_WARN_US
                      ? HealthLevel::PASS : HealthLevel::WARN;

  result.wifi = wifiEnabled ? HealthLevel::PASS : HealthLevel::NOT_TESTED;
  result.watchdog = esp_reset_reason() == ESP_RST_PANIC ||
                            esp_reset_reason() == ESP_RST_WDT
                        ? HealthLevel::WARN : HealthLevel::PASS;

  signals.setAnalog(state.mapVoltage, state.tpsVoltage);
  state.rpm = previousRpm;
  state.running = wasRunning;
  result.testDurationMs = millis() - started;
  return result;
}

String BoardHealthMonitor::toJson(const BoardHealth& h) const {
  String out = "{";
  out += "\"map\":\"" + String(healthText(h.adcMap)) + "\",";
  out += "\"mapV\":" + String(h.mapMeasuredV, 3) + ",";
  out += "\"tps\":\"" + String(healthText(h.adcTps)) + "\",";
  out += "\"tpsV\":" + String(h.tpsMeasuredV, 3) + ",";
  out += "\"timing\":\"" + String(healthText(h.timing)) + "\",";
  out += "\"jitterUs\":" + String(h.maxLoopJitterUs) + ",";
  out += "\"memory\":\"" + String(healthText(h.memory)) + "\",";
  out += "\"heap\":" + String(h.freeHeapBytes) + ",";
  out += "\"wifi\":\"" + String(healthText(h.wifi)) + "\",";
  out += "\"watchdog\":\"" + String(healthText(h.watchdog)) + "\",";
  out += "\"durationMs\":" + String(h.testDurationMs) + "}";
  return out;
}

String BoardHealthMonitor::toText(const BoardHealth& h) const {
  String out = "\n=== ESP32 DAC BOARD HEALTH ===\n";
  out += "DAC25->ADC34 MAP : " + String(healthText(h.adcMap)) +
         " measured=" + String(h.mapMeasuredV, 3) + "V\n";
  out += "DAC26->ADC35 TPS : " + String(healthText(h.adcTps)) +
         " measured=" + String(h.tpsMeasuredV, 3) + "V\n";
  out += "Timing jitter    : " + String(healthText(h.timing)) +
         " max=" + String(h.maxLoopJitterUs) + "us\n";
  out += "Free heap        : " + String(healthText(h.memory)) +
         " " + String(h.freeHeapBytes) + " bytes\n";
  out += "WiFi             : " + String(healthText(h.wifi)) + "\n";
  out += "Reset/watchdog   : " + String(healthText(h.watchdog)) + "\n";
  out += "NOTE: DAC feedback requires jumpers GPIO25->34 and GPIO26->35.\n";
  return out;
}
