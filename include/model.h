#pragma once

#include <Arduino.h>

enum class HealthLevel : uint8_t { PASS, WARN, FAIL, NOT_TESTED };

struct SimulatorState {
  bool running = false;
  bool webClientSeen = false;
  uint16_t rpm = 800;
  float mapVoltage = 1.20f;
  float tpsVoltage = 0.65f;
  uint8_t profile = 0;
  uint32_t lastClientMs = 0;
};

struct BoardHealth {
  HealthLevel adcMap = HealthLevel::NOT_TESTED;
  HealthLevel adcTps = HealthLevel::NOT_TESTED;
  HealthLevel timing = HealthLevel::NOT_TESTED;
  HealthLevel memory = HealthLevel::NOT_TESTED;
  HealthLevel wifi = HealthLevel::NOT_TESTED;
  HealthLevel watchdog = HealthLevel::NOT_TESTED;
  float mapCommandV = 0;
  float mapMeasuredV = 0;
  float tpsCommandV = 0;
  float tpsMeasuredV = 0;
  uint32_t maxLoopJitterUs = 0;
  uint32_t freeHeapBytes = 0;
  int8_t wifiRssi = 0;
  uint32_t testDurationMs = 0;
  uint32_t testCount = 0;
};

const char* healthText(HealthLevel level);

