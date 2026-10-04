#pragma once
#include <Arduino.h>

namespace Config {
constexpr char FW_VERSION[] = "0.4.0-dev";
constexpr char AP_SSID[] = "Mytech_simulator_v1.3";
constexpr char AP_PASSWORD[] = "12345678";
const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_GATEWAY(192, 168, 4, 1);
const IPAddress AP_MASK(255, 255, 255, 0);

constexpr uint8_t LED_PIN = 2;
constexpr uint8_t SCOPE_PIN = 4;
constexpr uint8_t CKP_PIN = 18;
constexpr uint8_t CMP_PIN = 19;
constexpr uint8_t TPS_PIN = 25;
constexpr uint8_t MAP_PIN = 26;
constexpr uint8_t ECT_PIN = 27;
constexpr uint8_t IAT_PIN = 32;
constexpr uint8_t O2_PIN = 33;
constexpr uint16_t MAX_RPM = 8000;
}
