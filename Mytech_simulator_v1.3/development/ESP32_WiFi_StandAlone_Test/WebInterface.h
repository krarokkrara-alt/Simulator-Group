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
  char controlBody[257] = {};
  size_t controlBodyLength = 0;
  bool controlBodyComplete = false;
  bool controlBodyOverflow = false;
  bool startAccessPoint();
  void receiveControlBody(const HTTPRaw &raw);
  void handleSet();
};
