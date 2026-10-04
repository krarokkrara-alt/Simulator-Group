#pragma once
#include <Arduino.h>
#include "AppConfig.h"

struct SensorValues {
  uint8_t tps = 10, map = 40, ect = 50, iat = 40, o2 = 50;
};

class SimulatorState {
public:
  bool running = false, ckpEnabled = true, cmpEnabled = true;
  bool ledTest = false, scopeTest = false, selfTestActive = false;
  uint16_t rpm = 1000;
  uint8_t vehicleProfile = 0;
  SensorValues sensors;

  void begin();
  void setSensor(const String &name, int value);
  void startSelfTest();
  void updateSelfTest();
  String toJson() const;

private:
  uint32_t selfTestStartedAt = 0;
};
