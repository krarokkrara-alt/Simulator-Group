#include "AppConfig.h"
#include "SimulatorState.h"
#include "SignalGenerator.h"
#include "WebInterface.h"

SimulatorState simulator;
SignalGenerator signals(simulator);
WebInterface web(simulator, signals);

void setup() {
  Serial.begin(115200); // diagnostics only; the UI is Wi-Fi
  delay(1500);          // allow USB/external power to stabilize on a cold start
  simulator.begin();
  signals.begin();
  web.begin();
}

void loop() {
  web.handleClient();
  simulator.updateSelfTest();
  signals.update();
  delay(1);
}
