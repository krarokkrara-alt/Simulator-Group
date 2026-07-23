#pragma once

#include "model.h"

class SignalEngine {
 public:
  void begin();
  void update(const SimulatorState& state);
  void allOff();
  void setAnalog(float mapVoltage, float tpsVoltage);
  uint32_t maxJitterUs() const { return maxJitterUs_; }
  void clearJitter() { maxJitterUs_ = 0; }

 private:
  uint32_t lastEdgeUs_ = 0;
  uint32_t nextEdgeUs_ = 0;
  uint32_t maxJitterUs_ = 0;
  uint16_t tooth_ = 0;
  bool edgeHigh_ = false;
};

