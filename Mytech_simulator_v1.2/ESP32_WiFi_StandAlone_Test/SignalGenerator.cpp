#include "SignalGenerator.h"
#include "AppConfig.h"

void SignalGenerator::begin() {
  pinMode(Config::CKP_PIN, OUTPUT);
  pinMode(Config::CMP_PIN, OUTPUT);
  digitalWrite(Config::CKP_PIN, LOW);
  digitalWrite(Config::CMP_PIN, LOW);
}

void SignalGenerator::update() {
  const uint32_t now = micros();
  if (!state.running || state.rpm == 0) {
    digitalWrite(Config::CKP_PIN, LOW);
    digitalWrite(Config::CMP_PIN, LOW);
    tooth = 0;
  } else {
    // 60-2-style timing basis: 120 edges per crank revolution.
    const uint32_t edgePeriod = 60000000UL / ((uint32_t)state.rpm * 120UL);
    if (now - lastCkpUs >= edgePeriod) {
      lastCkpUs = now;
      tooth = (tooth + 1) % 120;
      const bool missingTooth = tooth >= 116;
      ckpLevel = !ckpLevel;
      digitalWrite(Config::CKP_PIN, state.ckpEnabled && !missingTooth ? ckpLevel : LOW);
      digitalWrite(Config::CMP_PIN, state.cmpEnabled && tooth < 30 ? HIGH : LOW);
    }
  }

  if ((state.scopeTest || state.selfTestActive) && now - lastScopeUs >= 500) {
    lastScopeUs = now;
    scopeLevel = !scopeLevel;
    digitalWrite(Config::SCOPE_PIN, scopeLevel);
  } else if (!state.scopeTest && !state.selfTestActive) {
    digitalWrite(Config::SCOPE_PIN, LOW);
  }
  if (!state.selfTestActive) digitalWrite(Config::LED_PIN, state.ledTest);
}
