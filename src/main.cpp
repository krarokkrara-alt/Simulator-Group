#include <Arduino.h>
#include "board_health.h"
#include "config.h"
#include "model.h"
#include "signal_engine.h"
#include "web_ui.h"

SimulatorState state;
SignalEngine signals;
BoardHealthMonitor healthMonitor;
WebUi webUi;
uint32_t lastControlUs;
bool lastButton = true;
uint32_t lastVariantActionMs;
uint8_t presetIndex;

void updateStandaloneControls() {
  const bool pressed = digitalRead(Config::START_STOP_PIN) == LOW;
  if (pressed && lastButton) {
    state.running = !state.running;
    if (!state.running) signals.allOff();
  }
  lastButton = !pressed;

#if VARIANT_ID == 1
  state.rpm = map(analogRead(Config::RPM_POT_PIN), 0, 4095, Config::RPM_MIN, Config::RPM_MAX);
  state.mapVoltage = analogRead(Config::LOAD_POT_PIN) * 3.3f / 4095.0f;
  state.tpsVoltage = constrain(0.35f + state.mapVoltage * 0.75f, 0.0f, 3.3f);
#elif VARIANT_ID == 2
  // Two-button mode: GPIO32 Start/Stop, GPIO33 increases RPM in 250 RPM steps.
  if (digitalRead(Config::MODE_PIN) == LOW && millis() - lastVariantActionMs > 300) {
    state.rpm += 250;
    if (state.rpm > Config::RPM_MAX) state.rpm = Config::RPM_MIN;
    lastVariantActionMs = millis();
  }
  state.mapVoltage = 1.20f;
  state.tpsVoltage = 0.70f;
#elif VARIANT_ID == 3
  // Preset mode: idle, cruise, medium load, high load.
  static const uint16_t rpmPreset[] = {800, 1500, 2500, 4000};
  static const float mapPreset[] = {0.75f, 1.20f, 1.85f, 2.60f};
  if (digitalRead(Config::MODE_PIN) == LOW && millis() - lastVariantActionMs > 300) {
    presetIndex = (presetIndex + 1) % 4;
    lastVariantActionMs = millis();
  }
  state.rpm = rpmPreset[presetIndex];
  state.mapVoltage = mapPreset[presetIndex];
  state.tpsVoltage = constrain(0.30f + state.mapVoltage, 0.0f, 3.3f);
#elif VARIANT_ID == 4
  // Diagnostic mode keeps manual pots and emits a compact report every second.
  state.rpm = map(analogRead(Config::RPM_POT_PIN), 0, 4095, Config::RPM_MIN, Config::RPM_MAX);
  state.mapVoltage = analogRead(Config::LOAD_POT_PIN) * 3.3f / 4095.0f;
  state.tpsVoltage = constrain(0.35f + state.mapVoltage * 0.75f, 0.0f, 3.3f);
  if (millis() - lastVariantActionMs > 1000) {
    Serial.printf("RUN=%d RPM=%u MAP=%.2f TPS=%.2f JITTER=%luus HEAP=%lu\n",
                  state.running, state.rpm, state.mapVoltage, state.tpsVoltage,
                  signals.maxJitterUs(), ESP.getFreeHeap());
    lastVariantActionMs = millis();
  }
#elif VARIANT_ID == 5
  // Burn-in mode sweeps 800..4000 RPM and analog load continuously.
  state.running = true;
  const uint32_t phase = millis() % 30000UL;
  state.rpm = phase < 15000 ? map(phase, 0, 15000, 800, 4000)
                            : map(phase, 15000, 30000, 4000, 800);
  state.mapVoltage = 0.65f + (state.rpm - 800) * 2.1f / 3200.0f;
  state.tpsVoltage = 0.40f + (state.rpm - 800) * 2.3f / 3200.0f;
#endif
}

void setup() {
  Serial.begin(115200);
  pinMode(Config::STATUS_LED_PIN, OUTPUT);
  pinMode(Config::START_STOP_PIN, INPUT_PULLUP);
  pinMode(Config::MODE_PIN, INPUT_PULLUP);
  signals.begin();
  healthMonitor.begin();
  state.rpm = Config::RPM_BOOT;

#if APP_MODE == 1
  webUi.begin(state, signals, healthMonitor);
  Serial.printf("Web variant %d ready: connect to PUK_SIM_%d, open http://192.168.4.1\n",
                VARIANT_ID, VARIANT_ID);
#else
  Serial.printf("Standalone variant %d ready. Hold MODE at boot or send T for self-test.\n", VARIANT_ID);
#endif

  if (digitalRead(Config::MODE_PIN) == LOW) {
    const BoardHealth h = healthMonitor.runFullTest(signals, state, APP_MODE == 1);
    Serial.print(healthMonitor.toText(h));
  }
}

void loop() {
#if APP_MODE == 1
  webUi.loop();
  if (state.webClientSeen && millis() - state.lastClientMs > Config::CLIENT_TIMEOUT_MS) {
    state.running = false;
    signals.allOff();
  }
#else
  updateStandaloneControls();
#endif

  if (Serial.available()) {
    const char c = Serial.read();
    if (c == 'T' || c == 't') {
      const BoardHealth h = healthMonitor.runFullTest(signals, state, APP_MODE == 1);
      Serial.print(healthMonitor.toText(h));
    } else if (c == 'S' || c == 's') {
      state.running = true;
    } else if (c == 'X' || c == 'x') {
      state.running = false;
      signals.allOff();
    }
  }

  signals.update(state);
  digitalWrite(Config::STATUS_LED_PIN, state.running && (millis() / 250) % 2);
  delay(0);
}
