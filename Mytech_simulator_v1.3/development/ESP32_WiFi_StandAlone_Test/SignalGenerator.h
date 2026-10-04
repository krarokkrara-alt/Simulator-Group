#pragma once
#include <Arduino.h>
#include "SimulatorState.h"
#include "VehicleProfiles.h"

class SignalGenerator {
public:
  explicit SignalGenerator(SimulatorState &state) : state(state) {}
  void begin();
  // Called only by the Arduino loop task and its synchronous HTTP handlers.
  void update();
private:
  SimulatorState &state;
  hw_timer_t *crankTimer = nullptr, *scopeTimer = nullptr;
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  volatile bool active = false, ckpEnabled = false, cmpEnabled = false;
  volatile bool scopeEnabled = false, selfTestScope = false;
  volatile uint16_t edge = 0, selfTestTicks = 0;
  volatile bool scopeLevel = false;
  // Active trigger table for the selected profile; replaced only under mux while disarmed.
  uint8_t slotTable[MAX_TRIGGER_SLOTS] = {};
  volatile uint16_t slotCount = 0;
  uint8_t stagedTable[MAX_TRIGGER_SLOTS] = {};
  uint16_t appliedRpm = 0;
  int16_t appliedProfile = -1;
  uint8_t appliedTeeth = 0;
  uint32_t appliedSelfTestSequence = 0;
  bool appliedRunning = false, appliedSelfTest = false;
  // Explicit placement: ARDUINO_ISR_ATTR is empty in this core's default config.
  // This does not enable GPTIMER cache-safe interrupt allocation.
  static void IRAM_ATTR crankInterrupt(void *arg);
  static void IRAM_ATTR scopeInterrupt(void *arg);
};
