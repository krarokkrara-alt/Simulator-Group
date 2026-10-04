#include "SignalGenerator.h"
#include "AppConfig.h"
#include "soc/gpio_struct.h"

#if !defined(CONFIG_IDF_TARGET_ESP32)
#error "This development candidate targets classic ESP32 only; verify the actual chip first."
#endif

namespace {
constexpr uint32_t ckpMask = 1UL << Config::CKP_PIN;
constexpr uint32_t cmpMask = 1UL << Config::CMP_PIN;
constexpr uint32_t scopeMask = 1UL << Config::SCOPE_PIN;
static_assert(Config::CKP_PIN < 32 && Config::CMP_PIN < 32 && Config::SCOPE_PIN < 32,
              "Direct GPIO register masks require pins below 32");
}

void IRAM_ATTR SignalGenerator::crankInterrupt(void *arg) {
  auto *self = static_cast<SignalGenerator *>(arg);
  portENTER_CRITICAL_ISR(&self->mux);
  uint32_t high = 0;
  if (self->active && self->slotCount) {
    // One table byte per half-tooth slot of the selected profile's 720-degree cycle.
    const uint16_t slot = self->edge;
    const uint8_t bits = self->slotTable[slot];
    if (self->ckpEnabled && (bits & 1)) high |= ckpMask;
    if (self->cmpEnabled && (bits & 2)) high |= cmpMask;
    self->edge = slot + 1 >= self->slotCount ? 0 : slot + 1;
  }
  GPIO.out_w1tc = (ckpMask | cmpMask) & ~high;
  GPIO.out_w1ts = high;
  portEXIT_CRITICAL_ISR(&self->mux);
}

void IRAM_ATTR SignalGenerator::scopeInterrupt(void *arg) {
  auto *self = static_cast<SignalGenerator *>(arg);
  portENTER_CRITICAL_ISR(&self->mux);
  const bool enabled = self->scopeEnabled || (self->selfTestScope && self->selfTestTicks > 0);
  if (self->selfTestTicks > 0) --self->selfTestTicks;
  self->scopeLevel = enabled ? !self->scopeLevel : false;
  if (self->scopeLevel) GPIO.out_w1ts = scopeMask;
  else GPIO.out_w1tc = scopeMask;
  portEXIT_CRITICAL_ISR(&self->mux);
}

void SignalGenerator::begin() {
  for (uint8_t pin : {Config::CKP_PIN, Config::CMP_PIN, Config::SCOPE_PIN}) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
  crankTimer = timerBegin(1000000);
  scopeTimer = timerBegin(1000000);
  if (!crankTimer || !scopeTimer) {
    if (crankTimer) timerEnd(crankTimer);
    if (scopeTimer) timerEnd(scopeTimer);
    crankTimer = scopeTimer = nullptr;
    state.running = false;
    Serial.println("Hardware timer allocation failed; waveform outputs disabled.");
    return;
  }
  timerStop(crankTimer);
  timerAttachInterruptArg(crankTimer, crankInterrupt, this);
  timerAttachInterruptArg(scopeTimer, scopeInterrupt, this);
  timerAlarm(scopeTimer, 500, true, 0); // 500 us half-period = 1 kHz.
}

void SignalGenerator::update() {
  if (!crankTimer || !scopeTimer) {
    state.running = false;
    return;
  }
  const bool run = state.running && state.rpm > 0;
  const bool profileChanged = state.vehicleProfile != appliedProfile;
  const bool reconfigure = profileChanged || run != appliedRunning || (run && state.rpm != appliedRpm);
  if (reconfigure) {
    const auto &pattern = VehicleProfiles::get(state.vehicleProfile).pattern;
    const uint16_t stagedCount = profileChanged ? VehicleProfiles::buildSlotTable(pattern, stagedTable) : 0;
    // Disarm under the same lock as ISR GPIO writes, so STOP cannot race an edge.
    // A new profile restarts the cycle at slot 0 (first tooth after the gap).
    portENTER_CRITICAL(&mux);
    active = false;
    edge = 0;
    if (profileChanged) {
      memcpy(slotTable, stagedTable, stagedCount);
      slotCount = stagedCount;
    }
    GPIO.out_w1tc = ckpMask | cmpMask;
    portEXIT_CRITICAL(&mux);
    timerStop(crankTimer);
    if (profileChanged) {
      appliedProfile = state.vehicleProfile;
      appliedTeeth = pattern.teeth;
    }
    if (run) {
      // Half-tooth period: one revolution has 2 x teeth slots.
      const uint64_t slotsPerRev = uint64_t(appliedTeeth) * 2ULL;
      const uint64_t periodUs = (60000000ULL + uint64_t(state.rpm) * slotsPerRev / 2ULL) /
                                (uint64_t(state.rpm) * slotsPerRev);
      timerWrite(crankTimer, 0);
      timerAlarm(crankTimer, periodUs, true, 0);
    }
  }
  portENTER_CRITICAL(&mux);
  ckpEnabled = state.ckpEnabled;
  cmpEnabled = state.cmpEnabled;
  active = run;
  if (!run || !ckpEnabled) GPIO.out_w1tc = ckpMask;
  if (!run || !cmpEnabled) GPIO.out_w1tc = cmpMask;
  // Manual scope test and timed self-test are independent of START/STOP.
  scopeEnabled = state.scopeTest && !state.selfTestActive;
  selfTestScope = state.selfTestActive;
  if (state.selfTestActive && (!appliedSelfTest || state.selfTestSequence != appliedSelfTestSequence)) {
    selfTestTicks = 6000;
    scopeLevel = false;
    GPIO.out_w1tc = scopeMask;
  }
  if (!state.selfTestActive) selfTestTicks = 0;
  if (!scopeEnabled && !selfTestScope) {
    scopeLevel = false;
    GPIO.out_w1tc = scopeMask;
  }
  portEXIT_CRITICAL(&mux);
  if (reconfigure && run) timerStart(crankTimer);
  appliedRunning = run;
  appliedRpm = state.rpm;
  appliedSelfTest = state.selfTestActive;
  appliedSelfTestSequence = state.selfTestSequence;
  if (!state.selfTestActive) digitalWrite(Config::LED_PIN, state.ledTest);
}
