#pragma once
#include <WebServer.h>
#include "SimulatorState.h"
#include "SignalGenerator.h"

class WebInterface {
public:
  WebInterface(SimulatorState &state, SignalGenerator &signals) : state(state), signals(signals), server(80) {}
  void begin();
  void handleClient();
private:
  SimulatorState &state;
  SignalGenerator &signals;
  WebServer server;
  uint32_t lastHealthCheckMs = 0;
  bool startAccessPoint();
  void handleSet();
};
