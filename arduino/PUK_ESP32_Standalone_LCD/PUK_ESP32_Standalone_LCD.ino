#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

/*
  PUK ESP32 Standalone LCD ECU Simulator v1.0.0
  Hardware: classic ESP32 (internal DAC on GPIO25/26), LCD 20x4 I2C.

  IMPORTANT
  - CKP/CMP is a generic 36-1 training profile, not an ECU-specific profile.
  - GPIO is 3.3 V only. Use the documented protected interface.
  - ECU health mode detects generic responses; it does not certify the whole ECU.
*/

namespace Pin {
constexpr uint8_t DAC_MAP = 25;
constexpr uint8_t DAC_TPS = 26;
constexpr uint8_t ADC_MAP_FB = 34;
constexpr uint8_t ADC_TPS_FB = 35;
constexpr uint8_t CKP = 18;
constexpr uint8_t CMP = 19;
constexpr uint8_t START_STOP = 32;
constexpr uint8_t MODE = 33;
constexpr uint8_t TEST = 27;
constexpr uint8_t RPM_POT = 36;
constexpr uint8_t LOAD_POT = 39;
constexpr uint8_t ECU_RESP_A = 16; // active-low, optocoupler only
constexpr uint8_t ECU_RESP_B = 17; // active-low, optocoupler only
constexpr uint8_t LED = 2;
constexpr uint8_t SDA = 21;
constexpr uint8_t SCL = 22;
}

namespace Limit {
constexpr uint16_t RPM_MIN = 300;
constexpr uint16_t RPM_MAX = 5000;
constexpr float DAC_MAX_V = 3.30f;
constexpr float FEEDBACK_TOLERANCE_V = 0.22f;
constexpr uint32_t JITTER_WARN_US = 120;
}

enum class Mode : uint8_t { ROTARY, PRESET, DIAGNOSTIC, BURN_IN, ECU_HEALTH, COUNT };
enum class Grade : uint8_t { GOOD, WARNING, BAD, NOT_VERIFIED };

LiquidCrystal_I2C lcd(0x27, 20, 4);
Mode mode = Mode::ROTARY;
bool running = false;
uint16_t rpm = 800;
float mapV = 1.20f;
float tpsV = 0.65f;
uint8_t tooth = 0;
bool edgeHigh = false;
uint32_t nextEdgeUs = 0;
uint32_t lastEdgeUs = 0;
uint32_t maxJitterUs = 0;
uint32_t lastDisplayMs = 0;
uint8_t preset = 0;
volatile uint32_t responseCountA = 0;
volatile uint32_t responseCountB = 0;

struct Button {
  uint8_t pin;
  bool stable = HIGH;
  bool sampled = HIGH;
  uint32_t changedMs = 0;
  bool pressed() {
    const bool now = digitalRead(pin);
    if (now != sampled) { sampled = now; changedMs = millis(); }
    if (millis() - changedMs >= 30 && stable != sampled) {
      stable = sampled;
      return stable == LOW;
    }
    return false;
  }
};

Button startButton{Pin::START_STOP};
Button modeButton{Pin::MODE};
Button testButton{Pin::TEST};

void IRAM_ATTR responseAIsr() { responseCountA++; }
void IRAM_ATTR responseBIsr() { responseCountB++; }

const char* modeName(Mode value) {
  switch (value) {
    case Mode::ROTARY: return "ROTARY";
    case Mode::PRESET: return "PRESET";
    case Mode::DIAGNOSTIC: return "DIAGNOSTIC";
    case Mode::BURN_IN: return "BURN-IN";
    case Mode::ECU_HEALTH: return "ECU HEALTH";
    default: return "UNKNOWN";
  }
}

const char* gradeName(Grade value) {
  switch (value) {
    case Grade::GOOD: return "GOOD";
    case Grade::WARNING: return "CHECK";
    case Grade::BAD: return "BAD";
    default: return "NOT VERIFIED";
  }
}

uint8_t voltageToDac(float value) {
  value = constrain(value, 0.0f, Limit::DAC_MAX_V);
  return uint8_t(lroundf(value * 255.0f / Limit::DAC_MAX_V));
}

void setAnalogOutputs() {
  dacWrite(Pin::DAC_MAP, voltageToDac(mapV));
  dacWrite(Pin::DAC_TPS, voltageToDac(tpsV));
}

void allOff() {
  running = false;
  digitalWrite(Pin::CKP, LOW);
  digitalWrite(Pin::CMP, LOW);
  dacWrite(Pin::DAC_MAP, 0);
  dacWrite(Pin::DAC_TPS, 0);
  tooth = 0;
  edgeHigh = false;
  nextEdgeUs = micros();
}

void updateSignalEngine() {
  if (!running) return;
  setAnalogOutputs();
  const uint32_t slotUs = 60000000UL / (uint32_t(rpm) * 36UL);
  const uint32_t halfUs = max(70UL, slotUs / 2UL);
  const uint32_t now = micros();
  if (int32_t(now - nextEdgeUs) < 0) return;

  if (lastEdgeUs) {
    const uint32_t actual = now - lastEdgeUs;
    const uint32_t jitter = actual > halfUs ? actual - halfUs : halfUs - actual;
    maxJitterUs = max(maxJitterUs, jitter);
  }
  lastEdgeUs = now;
  nextEdgeUs += halfUs;
  edgeHigh = !edgeHigh;
  digitalWrite(Pin::CKP, tooth == 35 ? LOW : edgeHigh);
  digitalWrite(Pin::CMP, tooth < 9 ? HIGH : LOW);
  if (!edgeHigh) tooth = (tooth + 1) % 36;
}

float readFeedbackVoltage(uint8_t pin) {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < 24; i++) sum += analogRead(pin);
  return (sum / 24.0f) * 3.30f / 4095.0f;
}

void lcdLine(uint8_t row, const String& text) {
  String padded = text;
  while (padded.length() < 20) padded += ' ';
  lcd.setCursor(0, row);
  lcd.print(padded.substring(0, 20));
}

void showMainScreen() {
  lcdLine(0, "PUK SIM v1.0 " + String(running ? "RUN" : "STOP"));
  lcdLine(1, "MODE: " + String(modeName(mode)));
  lcdLine(2, "RPM:" + String(rpm) + " MAP:" + String(mapV, 2));
  lcdLine(3, "TPS:" + String(tpsV, 2) + " J:" + String(maxJitterUs) + "us");
}

void applyModeControls() {
  if (mode == Mode::ROTARY || mode == Mode::DIAGNOSTIC || mode == Mode::ECU_HEALTH) {
    rpm = map(analogRead(Pin::RPM_POT), 0, 4095, Limit::RPM_MIN, Limit::RPM_MAX);
    mapV = 0.40f + analogRead(Pin::LOAD_POT) * 2.80f / 4095.0f;
    tpsV = constrain(0.35f + mapV * 0.78f, 0.35f, 3.20f);
  } else if (mode == Mode::PRESET) {
    static const uint16_t rpmTable[] = {800, 1500, 2500, 4000};
    static const float mapTable[] = {0.75f, 1.20f, 1.85f, 2.70f};
    rpm = rpmTable[preset];
    mapV = mapTable[preset];
    tpsV = constrain(mapV + 0.25f, 0.40f, 3.20f);
  } else if (mode == Mode::BURN_IN) {
    running = true;
    const uint32_t phase = millis() % 30000UL;
    rpm = phase < 15000 ? map(phase, 0, 15000, 800, 4000)
                        : map(phase, 15000, 30000, 4000, 800);
    mapV = 0.65f + (rpm - 800) * 2.05f / 3200.0f;
    tpsV = 0.45f + (rpm - 800) * 2.30f / 3200.0f;
  }
}

Grade runHealthTest() {
  const bool oldRunning = running;
  const uint16_t oldRpm = rpm;
  const float oldMap = mapV, oldTps = tpsV;
  responseCountA = responseCountB = 0;
  maxJitterUs = 0;

  lcdLine(0, "HEALTH TEST RUNNING");
  lcdLine(1, "Keep E-STOP ready");
  lcdLine(2, "Stage 1/2: 800 RPM");
  rpm = 800; mapV = 1.00f; tpsV = 0.70f; running = true;
  uint32_t start = millis();
  while (millis() - start < 1500) updateSignalEngine();
  lcdLine(2, "Stage 2/2:1500 RPM");
  rpm = 1500; mapV = 1.80f; tpsV = 1.50f;
  start = millis();
  while (millis() - start < 1500) updateSignalEngine();
  allOff();

  // DAC feedback requires temporary GPIO25->34 and GPIO26->35 test jumpers.
  dacWrite(Pin::DAC_MAP, voltageToDac(1.00f));
  dacWrite(Pin::DAC_TPS, voltageToDac(2.00f));
  delay(30);
  const float mapFeedback = readFeedbackVoltage(Pin::ADC_MAP_FB);
  const float tpsFeedback = readFeedbackVoltage(Pin::ADC_TPS_FB);
  allOff();

  const bool dacTested = mapFeedback > 0.05f && tpsFeedback > 0.05f;
  const bool dacPass = fabsf(mapFeedback - 1.00f) <= Limit::FEEDBACK_TOLERANCE_V &&
                       fabsf(tpsFeedback - 2.00f) <= Limit::FEEDBACK_TOLERANCE_V;
  const bool responseTested = responseCountA > 0 || responseCountB > 0;
  Grade grade = Grade::NOT_VERIFIED;
  if ((dacTested && !dacPass) || maxJitterUs > Limit::JITTER_WARN_US * 2UL) grade = Grade::BAD;
  else if (dacTested && dacPass && responseTested && maxJitterUs <= Limit::JITTER_WARN_US) grade = Grade::GOOD;
  else if (dacTested || responseTested) grade = Grade::WARNING;

  lcdLine(0, "RESULT: " + String(gradeName(grade)));
  lcdLine(1, "DAC:" + String(dacTested ? (dacPass ? "PASS" : "FAIL") : "N/T"));
  lcdLine(2, "RESP A:" + String(responseCountA) + " B:" + String(responseCountB));
  lcdLine(3, "JITTER:" + String(maxJitterUs) + "us");
  Serial.printf("HEALTH,%s,DAC=%s,A=%lu,B=%lu,JITTER=%lu\n", gradeName(grade),
                dacTested ? (dacPass ? "PASS" : "FAIL") : "NOT_TESTED",
                responseCountA, responseCountB, maxJitterUs);
  delay(4000);

  rpm = oldRpm; mapV = oldMap; tpsV = oldTps; running = oldRunning;
  if (!running) allOff();
  return grade;
}

void setup() {
  Serial.begin(115200);
  pinMode(Pin::CKP, OUTPUT); pinMode(Pin::CMP, OUTPUT); pinMode(Pin::LED, OUTPUT);
  pinMode(Pin::START_STOP, INPUT_PULLUP); pinMode(Pin::MODE, INPUT_PULLUP);
  pinMode(Pin::TEST, INPUT_PULLUP);
  pinMode(Pin::ECU_RESP_A, INPUT_PULLUP); pinMode(Pin::ECU_RESP_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(Pin::ECU_RESP_A), responseAIsr, FALLING);
  attachInterrupt(digitalPinToInterrupt(Pin::ECU_RESP_B), responseBIsr, FALLING);
  analogReadResolution(12);
  analogSetPinAttenuation(Pin::ADC_MAP_FB, ADC_11db);
  analogSetPinAttenuation(Pin::ADC_TPS_FB, ADC_11db);
  Wire.begin(Pin::SDA, Pin::SCL);
  lcd.init(); lcd.backlight();
  allOff();
  lcdLine(0, "PUK ECU SIMULATOR"); lcdLine(1, "Standalone LCD");
  lcdLine(2, "Outputs OFF"); lcdLine(3, "v1.0.0");
  delay(1500);
}

void loop() {
  if (startButton.pressed() && mode != Mode::BURN_IN) {
    running = !running;
    if (!running) allOff();
  }
  if (modeButton.pressed()) {
    if (mode == Mode::PRESET && running) preset = (preset + 1) % 4;
    else {
      allOff();
      mode = Mode((uint8_t(mode) + 1) % uint8_t(Mode::COUNT));
    }
  }
  if (testButton.pressed()) runHealthTest();

  applyModeControls();
  updateSignalEngine();
  digitalWrite(Pin::LED, running && ((millis() / 250) % 2));
  if (millis() - lastDisplayMs >= 200) {
    showMainScreen();
    lastDisplayMs = millis();
  }
  delay(0);
}
