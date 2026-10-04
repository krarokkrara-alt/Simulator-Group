#pragma once
#include <Arduino.h>
#include "SimulatorState.h"

class SignalGenerator {
public:
  explicit SignalGenerator(SimulatorState &state) : state(state) {}
  void begin();
  void update();
private:
  SimulatorState &state;
  uint32_t lastCkpUs = 0, lastScopeUs = 0;
  uint16_t tooth = 0;
  bool ckpLevel = false, scopeLevel = false;
};
