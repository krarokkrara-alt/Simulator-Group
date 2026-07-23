#pragma once

#include <Arduino.h>
#include "board_health.h"
#include "model.h"
#include "signal_engine.h"

class WebUi {
 public:
  void begin(SimulatorState& state, SignalEngine& signals, BoardHealthMonitor& monitor);
  void loop();
  int8_t rssi() const;

 private:
  void* server_ = nullptr;
};

