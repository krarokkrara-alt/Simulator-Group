#include "signal_engine.h"
#include "config.h"

namespace {
uint8_t voltageToDac(float voltage) {
  voltage = constrain(voltage, 0.0f, Config::DAC_NOMINAL_FULL_SCALE_V);
  return static_cast<uint8_t>(roundf(voltage * 255.0f / Config::DAC_NOMINAL_FULL_SCALE_V));
}
}

void SignalEngine::begin() {
  pinMode(Config::CKP_PIN, OUTPUT);
  pinMode(Config::CMP_PIN, OUTPUT);
  allOff();
  setAnalog(0.0f, 0.0f);
}

void SignalEngine::setAnalog(float mapVoltage, float tpsVoltage) {
  dacWrite(Config::DAC_MAP_PIN, voltageToDac(mapVoltage));
  dacWrite(Config::DAC_TPS_PIN, voltageToDac(tpsVoltage));
}

void SignalEngine::allOff() {
  digitalWrite(Config::CKP_PIN, LOW);
  digitalWrite(Config::CMP_PIN, LOW);
  tooth_ = 0;
  edgeHigh_ = false;
  nextEdgeUs_ = micros();
}

void SignalEngine::update(const SimulatorState& state) {
  setAnalog(state.mapVoltage, state.tpsVoltage);
  if (!state.running || state.rpm < Config::RPM_MIN) {
    allOff();
    return;
  }

  // Generic 36-1 demonstration profile. Verify the target ECU pattern before use.
  const uint32_t toothPeriodUs = 60000000UL / (static_cast<uint32_t>(state.rpm) * 36UL);
  const uint32_t calculatedHalfPeriodUs = toothPeriodUs / 2;
  const uint32_t halfPeriodUs = calculatedHalfPeriodUs < 70 ? 70 : calculatedHalfPeriodUs;
  const uint32_t now = micros();
  if (static_cast<int32_t>(now - nextEdgeUs_) < 0) return;

  if (lastEdgeUs_ != 0) {
    const uint32_t actual = now - lastEdgeUs_;
    const uint32_t jitter = actual > halfPeriodUs ? actual - halfPeriodUs : halfPeriodUs - actual;
    maxJitterUs_ = max(maxJitterUs_, jitter);
  }
  lastEdgeUs_ = now;
  nextEdgeUs_ += halfPeriodUs;
  edgeHigh_ = !edgeHigh_;

  const bool missingTooth = tooth_ == 35;
  digitalWrite(Config::CKP_PIN, missingTooth ? LOW : edgeHigh_);
  digitalWrite(Config::CMP_PIN, (tooth_ < 9) ? HIGH : LOW);
  if (!edgeHigh_) tooth_ = (tooth_ + 1) % 36;
}
