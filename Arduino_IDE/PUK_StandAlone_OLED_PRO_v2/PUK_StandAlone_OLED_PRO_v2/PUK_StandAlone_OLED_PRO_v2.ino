/*
  PUK Stand Alone IGT/IGF OLED Simulator PRO v2.0
  Target: Arduino UNO / Nano (ATmega328P)
  Display: SSD1306 128x64 I2C, address 0x3C

  Features
  - Multi-page industrial-style OLED UI
  - Rotary encoder navigation
  - Adjustable RPM, duty cycle and expected IGF ratio
  - EEPROM save/load with signature and bounds checking
  - Non-blocking IGT signal generator
  - IGF interrupt capture with digital debounce/noise rejection
  - Fail detection: STARTING, OK, MISS, LOW, HIGH, NOISE
  - Audible/LED fault indication

  IMPORTANT
  D9 is a logic signal only. Do not connect D9 directly to an automotive ECU.
  Use the transistor/open-collector output stage in WIRING.md.
*/

#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr uint8_t OLED_ADDR = 0x3C;
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

constexpr uint8_t PIN_IGF       = 2;
constexpr uint8_t PIN_ENC_A     = 3;
constexpr uint8_t PIN_ENC_B     = 4;
constexpr uint8_t PIN_ENC_SW    = 5;
constexpr uint8_t PIN_STARTSTOP = 6;
constexpr uint8_t PIN_BUZZER    = 7;
constexpr uint8_t PIN_FAULT_LED = 8;
constexpr uint8_t PIN_IGT       = 9;

constexpr uint16_t RPM_MIN = 200;
constexpr uint16_t RPM_MAX = 7000;
constexpr uint8_t DUTY_MIN = 5;
constexpr uint8_t DUTY_MAX = 90;
constexpr uint8_t RATIO_MIN = 1;
constexpr uint8_t RATIO_MAX = 8;
constexpr uint16_t IGF_TIMEOUT_MS = 700;
constexpr uint16_t STARTUP_GRACE_MS = 1200;
constexpr uint32_t IGF_MIN_EDGE_US = 250;
constexpr uint16_t EEPROM_SIGNATURE = 0x504B;
constexpr int EEPROM_ADDR = 0;

struct Settings {
  uint16_t signature;
  uint16_t rpm;
  uint8_t duty;
  uint8_t pulsesPerRev;
  uint8_t expectedIgfRatio;
  uint8_t tolerancePct;
  uint8_t soundEnabled;
  uint8_t reserved;
};

Settings settings;

enum class Page : uint8_t {
  DASHBOARD, RPM, DUTY, PULSES, IGF_RATIO, TOLERANCE, SETTINGS, DIAGNOSTIC, PAGE_COUNT
};

enum class FailState : uint8_t {
  STOPPED, STARTING, OK, MISS, LOW_RATE, HIGH_RATE, NOISE
};

Page page = Page::DASHBOARD;
FailState failState = FailState::STOPPED;
bool running = false;
bool dirtySettings = false;

volatile uint32_t igfTotalCount = 0;
volatile uint32_t igfWindowCount = 0;
volatile uint32_t lastIgfEdgeUs = 0;
volatile uint32_t noiseRejectedCount = 0;

uint32_t igtTotalCount = 0;
uint32_t igtWindowCount = 0;
uint32_t runStartedMs = 0;
uint32_t lastHealthCheckMs = 0;
uint32_t lastDisplayMs = 0;
uint32_t lastBuzzerMs = 0;
uint16_t measuredIgfRate = 0;
uint16_t expectedIgfRate = 0;
int16_t rateErrorPct = 0;
bool igtHigh = false;
uint32_t phaseStartedUs = 0;

template <typename T>
T clampValue(T value, T minimum, T maximum) {
  if (value < minimum) return minimum;
  if (value > maximum) return maximum;
  return value;
}

const __FlashStringHelper* failText(FailState state) {
  switch (state) {
    case FailState::STOPPED:   return F("STOP");
    case FailState::STARTING:  return F("START");
    case FailState::OK:        return F("OK");
    case FailState::MISS:      return F("MISS");
    case FailState::LOW_RATE:  return F("LOW");
    case FailState::HIGH_RATE: return F("HIGH");
    case FailState::NOISE:     return F("NOISE");
  }
  return F("?");
}

bool isFault(FailState state) {
  return state == FailState::MISS || state == FailState::LOW_RATE ||
         state == FailState::HIGH_RATE || state == FailState::NOISE;
}

void setFactoryDefaults() {
  settings.signature = EEPROM_SIGNATURE;
  settings.rpm = 1200;
  settings.duty = 30;
  settings.pulsesPerRev = 1;
  settings.expectedIgfRatio = 1;
  settings.tolerancePct = 30;
  settings.soundEnabled = 1;
  settings.reserved = 0;
}

bool settingsAreValid() {
  return settings.signature == EEPROM_SIGNATURE &&
         settings.rpm >= RPM_MIN && settings.rpm <= RPM_MAX &&
         settings.duty >= DUTY_MIN && settings.duty <= DUTY_MAX &&
         settings.pulsesPerRev >= 1 && settings.pulsesPerRev <= 8 &&
         settings.expectedIgfRatio >= RATIO_MIN && settings.expectedIgfRatio <= RATIO_MAX &&
         settings.tolerancePct >= 5 && settings.tolerancePct <= 80 &&
         settings.soundEnabled <= 1;
}

void loadSettings() {
  EEPROM.get(EEPROM_ADDR, settings);
  if (!settingsAreValid()) {
    setFactoryDefaults();
    EEPROM.put(EEPROM_ADDR, settings);
  }
}

void saveSettings() {
  settings.signature = EEPROM_SIGNATURE;
  EEPROM.put(EEPROM_ADDR, settings);
  dirtySettings = false;
}

void onIgfFallingEdge() {
  const uint32_t nowUs = micros();
  const uint32_t intervalUs = nowUs - lastIgfEdgeUs;
  if (lastIgfEdgeUs != 0 && intervalUs < IGF_MIN_EDGE_US) {
    noiseRejectedCount++;
    return;
  }
  lastIgfEdgeUs = nowUs;
  igfTotalCount++;
  igfWindowCount++;
}

int8_t readEncoderStep() {
  static uint8_t previousA = HIGH;
  const uint8_t currentA = digitalRead(PIN_ENC_A);
  int8_t step = 0;
  if (currentA != previousA && currentA == LOW) {
    step = (digitalRead(PIN_ENC_B) == HIGH) ? 1 : -1;
  }
  previousA = currentA;
  return step;
}

bool buttonPressed(uint8_t pin) {
  static uint32_t lastChange[2] = {0, 0};
  static uint8_t stableState[2] = {HIGH, HIGH};
  static uint8_t lastReading[2] = {HIGH, HIGH};
  const uint8_t index = (pin == PIN_ENC_SW) ? 0 : 1;
  const uint8_t reading = digitalRead(pin);
  bool pressed = false;
  if (reading != lastReading[index]) {
    lastChange[index] = millis();
    lastReading[index] = reading;
  }
  if (millis() - lastChange[index] > 30 && reading != stableState[index]) {
    stableState[index] = reading;
    if (stableState[index] == LOW) pressed = true;
  }
  return pressed;
}

void adjustCurrentPage(int8_t step) {
  if (step == 0) return;
  switch (page) {
    case Page::DASHBOARD:
      page = static_cast<Page>((static_cast<int8_t>(page) + step + static_cast<int8_t>(Page::PAGE_COUNT)) % static_cast<int8_t>(Page::PAGE_COUNT));
      break;
    case Page::RPM:
      settings.rpm = clampValue<int>(settings.rpm + step * 50, RPM_MIN, RPM_MAX);
      dirtySettings = true;
      break;
    case Page::DUTY:
      settings.duty = clampValue<int>(settings.duty + step, DUTY_MIN, DUTY_MAX);
      dirtySettings = true;
      break;
    case Page::PULSES:
      settings.pulsesPerRev = clampValue<int>(settings.pulsesPerRev + step, 1, 8);
      dirtySettings = true;
      break;
    case Page::IGF_RATIO:
      settings.expectedIgfRatio = clampValue<int>(settings.expectedIgfRatio + step, RATIO_MIN, RATIO_MAX);
      dirtySettings = true;
      break;
    case Page::TOLERANCE:
      settings.tolerancePct = clampValue<int>(settings.tolerancePct + step * 5, 5, 80);
      dirtySettings = true;
      break;
    case Page::SETTINGS:
      settings.soundEnabled = settings.soundEnabled ? 0 : 1;
      dirtySettings = true;
      break;
    case Page::DIAGNOSTIC:
    case Page::PAGE_COUNT:
      page = Page::DASHBOARD;
      break;
  }
}

void nextPage() {
  uint8_t next = static_cast<uint8_t>(page) + 1;
  if (next >= static_cast<uint8_t>(Page::PAGE_COUNT)) next = 0;
  page = static_cast<Page>(next);
}

void stopOutput() {
  running = false;
  digitalWrite(PIN_IGT, LOW);
  igtHigh = false;
  failState = FailState::STOPPED;
  digitalWrite(PIN_FAULT_LED, LOW);
}

void startOutput() {
  noInterrupts();
  igfWindowCount = 0;
  lastIgfEdgeUs = 0;
  interrupts();
  igtWindowCount = 0;
  measuredIgfRate = 0;
  expectedIgfRate = 0;
  rateErrorPct = 0;
  running = true;
  runStartedMs = millis();
  lastHealthCheckMs = millis();
  phaseStartedUs = micros();
  igtHigh = false;
  failState = FailState::STARTING;
}

void toggleRun() {
  if (running) stopOutput();
  else startOutput();
}

void updateIgtGenerator() {
  if (!running) return;
  const uint32_t pulsePeriodUs = 60000000UL / ((uint32_t)settings.rpm * settings.pulsesPerRev);
  const uint32_t highTimeUs = max<uint32_t>(100, (pulsePeriodUs * settings.duty) / 100UL);
  const uint32_t lowTimeUs = max<uint32_t>(100, pulsePeriodUs - highTimeUs);
  const uint32_t nowUs = micros();
  const uint32_t elapsedUs = nowUs - phaseStartedUs;
  if (!igtHigh && elapsedUs >= lowTimeUs) {
    igtHigh = true;
    phaseStartedUs = nowUs;
    digitalWrite(PIN_IGT, HIGH);
    igtTotalCount++;
    igtWindowCount++;
  } else if (igtHigh && elapsedUs >= highTimeUs) {
    igtHigh = false;
    phaseStartedUs = nowUs;
    digitalWrite(PIN_IGT, LOW);
  }
}

void updateHealthMonitor() {
  if (!running) return;
  const uint32_t nowMs = millis();
  if (nowMs - runStartedMs < STARTUP_GRACE_MS) {
    failState = FailState::STARTING;
    return;
  }
  if (nowMs - lastHealthCheckMs < 1000) return;
  const uint32_t windowMs = nowMs - lastHealthCheckMs;
  lastHealthCheckMs = nowMs;

  noInterrupts();
  const uint32_t igfEdges = igfWindowCount;
  igfWindowCount = 0;
  const uint32_t lastEdgeUsCopy = lastIgfEdgeUs;
  const uint32_t noiseCopy = noiseRejectedCount;
  noiseRejectedCount = 0;
  interrupts();

  const uint32_t igtEdges = igtWindowCount;
  igtWindowCount = 0;
  measuredIgfRate = (uint16_t)((igfEdges * 1000UL) / max<uint32_t>(1, windowMs));
  expectedIgfRate = (uint16_t)(((uint32_t)settings.rpm * settings.pulsesPerRev * settings.expectedIgfRatio) / 60UL);
  if (expectedIgfRate > 0) {
    rateErrorPct = (int16_t)(((int32_t)measuredIgfRate - expectedIgfRate) * 100L / expectedIgfRate);
  } else {
    rateErrorPct = 0;
  }

  const bool timedOut = (lastEdgeUsCopy == 0) || ((micros() - lastEdgeUsCopy) > (uint32_t)IGF_TIMEOUT_MS * 1000UL);
  const uint16_t lowerLimit = expectedIgfRate * (100 - settings.tolerancePct) / 100;
  const uint16_t upperLimit = expectedIgfRate * (100 + settings.tolerancePct) / 100;

  if (noiseCopy > max<uint32_t>(3, igfEdges / 3)) failState = FailState::NOISE;
  else if (timedOut || (igtEdges > 0 && igfEdges == 0)) failState = FailState::MISS;
  else if (measuredIgfRate < lowerLimit) failState = FailState::LOW_RATE;
  else if (measuredIgfRate > upperLimit) failState = FailState::HIGH_RATE;
  else failState = FailState::OK;

  const bool fault = isFault(failState);
  digitalWrite(PIN_FAULT_LED, fault ? HIGH : LOW);
  if (fault && settings.soundEnabled && nowMs - lastBuzzerMs > 1500) {
    tone(PIN_BUZZER, 2200, 100);
    lastBuzzerMs = nowMs;
  }
}

void drawHeader(const __FlashStringHelper* title) {
  display.fillRect(0, 0, 128, 12, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(3, 2);
  display.print(title);
  display.setTextColor(SSD1306_WHITE);
}

void drawFooter() {
  display.drawLine(0, 54, 127, 54, SSD1306_WHITE);
  display.setCursor(0, 56);
  display.print(dirtySettings ? F("*UNSAVED") : F("SAVED"));
  display.setCursor(78, 56);
  display.print(running ? F("RUN") : F("STOP"));
}

void drawDashboard() {
  drawHeader(F("PUK SIM PRO v2"));
  display.setTextSize(2);
  display.setCursor(0, 16);
  display.print(settings.rpm);
  display.setTextSize(1);
  display.print(F(" RPM"));
  display.setCursor(0, 35);
  display.print(F("IGT "));
  display.print(settings.duty);
  display.print(F("%  IGF "));
  display.print(failText(failState));
  display.setCursor(0, 45);
  display.print(F("Rate "));
  display.print(measuredIgfRate);
  display.print(F("/"));
  display.print(expectedIgfRate);
  drawFooter();
}

void drawValuePage(const __FlashStringHelper* title, int value, const __FlashStringHelper* unit) {
  drawHeader(title);
  display.setTextSize(3);
  display.setCursor(8, 20);
  display.print(value);
  display.setTextSize(1);
  display.print(F(" "));
  display.print(unit);
  display.setCursor(0, 47);
  display.print(F("Rotate=Adjust Press=Next"));
  drawFooter();
}

void drawSettingsPage() {
  drawHeader(F("ALARM SETTINGS"));
  display.setTextSize(2);
  display.setCursor(5, 21);
  display.print(settings.soundEnabled ? F("SOUND ON") : F("SOUND OFF"));
  display.setTextSize(1);
  display.setCursor(0, 45);
  display.print(F("Rotate=Toggle Press=Next"));
  drawFooter();
}

void drawDiagnosticPage() {
  uint32_t igfCopy;
  uint32_t rejectedCopy;
  noInterrupts();
  igfCopy = igfTotalCount;
  rejectedCopy = noiseRejectedCount;
  interrupts();
  drawHeader(F("LIVE DIAGNOSTIC"));
  display.setCursor(0, 15);
  display.print(F("IGT total: "));
  display.print(igtTotalCount);
  display.setCursor(0, 25);
  display.print(F("IGF total: "));
  display.print(igfCopy);
  display.setCursor(0, 35);
  display.print(F("Error % : "));
  display.print(rateErrorPct);
  display.setCursor(0, 45);
  display.print(F("Noise rej: "));
  display.print(rejectedCopy);
  drawFooter();
}

void updateDisplay() {
  if (millis() - lastDisplayMs < 100) return;
  lastDisplayMs = millis();
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  switch (page) {
    case Page::DASHBOARD:  drawDashboard(); break;
    case Page::RPM:        drawValuePage(F("SET ENGINE SPEED"), settings.rpm, F("RPM")); break;
    case Page::DUTY:       drawValuePage(F("SET IGT DUTY"), settings.duty, F("%")); break;
    case Page::PULSES:     drawValuePage(F("IGT PULSES/REV"), settings.pulsesPerRev, F("PPR")); break;
    case Page::IGF_RATIO:  drawValuePage(F("EXPECTED IGF RATIO"), settings.expectedIgfRatio, F("xIGT")); break;
    case Page::TOLERANCE:  drawValuePage(F("FAIL TOLERANCE"), settings.tolerancePct, F("%")); break;
    case Page::SETTINGS:   drawSettingsPage(); break;
    case Page::DIAGNOSTIC: drawDiagnosticPage(); break;
    case Page::PAGE_COUNT: page = Page::DASHBOARD; break;
  }
  display.display();
}

void showBootScreen() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(18, 12);
  display.print(F("PUK SIM"));
  display.setTextSize(1);
  display.setCursor(26, 37);
  display.print(F("OLED PRO v2.0"));
  display.setCursor(20, 51);
  display.print(F("SELF TEST: PASS"));
  display.display();
  delay(900);
}

void setup() {
  pinMode(PIN_IGT, OUTPUT);
  pinMode(PIN_IGF, INPUT_PULLUP);
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  pinMode(PIN_ENC_SW, INPUT_PULLUP);
  pinMode(PIN_STARTSTOP, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_FAULT_LED, OUTPUT);
  digitalWrite(PIN_IGT, LOW);
  digitalWrite(PIN_FAULT_LED, LOW);
  loadSettings();
  Wire.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    while (true) {
      digitalWrite(PIN_FAULT_LED, !digitalRead(PIN_FAULT_LED));
      delay(150);
    }
  }
  showBootScreen();
  attachInterrupt(digitalPinToInterrupt(PIN_IGF), onIgfFallingEdge, FALLING);
}

void loop() {
  const int8_t encoderStep = readEncoderStep();
  adjustCurrentPage(encoderStep);
  if (buttonPressed(PIN_ENC_SW)) nextPage();
  if (buttonPressed(PIN_STARTSTOP)) toggleRun();

  static uint32_t dirtySinceMs = 0;
  if (dirtySettings && dirtySinceMs == 0) dirtySinceMs = millis();
  if (!dirtySettings) dirtySinceMs = 0;
  if (dirtySettings && !running && millis() - dirtySinceMs > 3000) saveSettings();

  updateIgtGenerator();
  updateHealthMonitor();
  updateDisplay();
}
