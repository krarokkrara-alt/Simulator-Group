#pragma once

#include <Arduino.h>

#ifndef APP_MODE
#define APP_MODE 0
#endif

#ifndef VARIANT_ID
#define VARIANT_ID 1
#endif

namespace Config {
constexpr uint8_t DAC_MAP_PIN = 25;
constexpr uint8_t DAC_TPS_PIN = 26;
constexpr uint8_t ADC_MAP_FEEDBACK_PIN = 34;
constexpr uint8_t ADC_TPS_FEEDBACK_PIN = 35;
constexpr uint8_t CKP_PIN = 18;
constexpr uint8_t CMP_PIN = 19;
constexpr uint8_t START_STOP_PIN = 32;
constexpr uint8_t MODE_PIN = 33;
constexpr uint8_t RPM_POT_PIN = 36;
constexpr uint8_t LOAD_POT_PIN = 39;
constexpr uint8_t STATUS_LED_PIN = 2;

constexpr uint16_t RPM_MIN = 100;
constexpr uint16_t RPM_MAX = 6000;
constexpr uint16_t RPM_BOOT = 800;
constexpr uint32_t CONTROL_PERIOD_US = 10000;
constexpr uint32_t CLIENT_TIMEOUT_MS = 10000;
constexpr float ADC_FULL_SCALE_V = 3.30f;
constexpr float DAC_NOMINAL_FULL_SCALE_V = 3.30f;
constexpr float FEEDBACK_TOLERANCE_V = 0.22f;
constexpr uint32_t MIN_HEAP_WARN_BYTES = 45000;
constexpr uint32_t JITTER_WARN_US = 120;
constexpr char AP_PASSWORD[] = "puk-sim-32";
}  // namespace Config

