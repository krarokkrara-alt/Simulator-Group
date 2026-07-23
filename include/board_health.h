#pragma once

#include "model.h"
#include "signal_engine.h"

class BoardHealthMonitor {
 public:
  void begin();
  BoardHealth runFullTest(SignalEngine& signals, SimulatorState& state, bool wifiEnabled);
  String toJson(const BoardHealth& health) const;
  String toText(const BoardHealth& health) const;

 private:
  float readVoltage(uint8_t pin);
  HealthLevel compareVoltage(float commanded, float measured) const;
};

