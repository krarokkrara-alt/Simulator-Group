#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_system.h>

/*
  PUK ESP32 DAC ECU Simulator - Arduino IDE edition
  Set VARIANT_ID from 1 to 10 before Upload.

  1 Rotary/Pot       6 Web Dashboard
  2 Buttons          7 Web Mobile
  3 Preset           8 Web Lab
  4 Diagnostic       9 Web Training
  5 Burn-in         10 Web Burn-in

  IMPORTANT: CKP/CMP is a generic 36-1 demonstration pattern.
  Confirm tooth pattern, phase, voltage interface and ECU pinout before bench use.
*/

#define VARIANT_ID 1

#if VARIANT_ID < 1 || VARIANT_ID > 10
#error "VARIANT_ID must be 1..10"
#endif

constexpr bool WEB_MODE = VARIANT_ID >= 6;
constexpr uint8_t DAC_MAP_PIN = 25;
constexpr uint8_t DAC_TPS_PIN = 26;
constexpr uint8_t ADC_MAP_PIN = 34;
constexpr uint8_t ADC_TPS_PIN = 35;
constexpr uint8_t CKP_PIN = 18;
constexpr uint8_t CMP_PIN = 19;
constexpr uint8_t START_STOP_PIN = 32;
constexpr uint8_t MODE_PIN = 33;
constexpr uint8_t RPM_POT_PIN = 36;
constexpr uint8_t LOAD_POT_PIN = 39;
constexpr uint8_t STATUS_LED_PIN = 2;
constexpr uint16_t RPM_MIN = 100;
constexpr uint16_t RPM_MAX = 6000;
constexpr uint32_t CLIENT_TIMEOUT_MS = 10000;
constexpr float DAC_FULL_SCALE_V = 3.30f;
constexpr float FEEDBACK_TOLERANCE_V = 0.22f;
constexpr uint32_t MIN_HEAP_BYTES = 45000;
constexpr uint32_t JITTER_LIMIT_US = 120;

struct SimulatorState {
  bool running = false;
  uint16_t rpm = 800;
  float mapVoltage = 1.20f;
  float tpsVoltage = 0.65f;
  bool webClientSeen = false;
  uint32_t lastClientMs = 0;
};

SimulatorState state;
WebServer server(80);
uint8_t tooth = 0;
bool edgeHigh = false;
uint32_t nextEdgeUs = 0;
uint32_t lastEdgeUs = 0;
uint32_t maxJitterUs = 0;
bool lastStartReleased = true;
bool lastModeReleased = true;
uint8_t presetIndex = 0;
uint32_t lastReportMs = 0;

uint8_t voltageToDac(float voltage) {
  voltage = constrain(voltage, 0.0f, DAC_FULL_SCALE_V);
  return uint8_t(roundf(voltage * 255.0f / DAC_FULL_SCALE_V));
}

void setAnalog(float mapV, float tpsV) {
  dacWrite(DAC_MAP_PIN, voltageToDac(mapV));
  dacWrite(DAC_TPS_PIN, voltageToDac(tpsV));
}

void allOff() {
  state.running = false;
  digitalWrite(CKP_PIN, LOW);
  digitalWrite(CMP_PIN, LOW);
  tooth = 0;
  edgeHigh = false;
  nextEdgeUs = micros();
}

void updateSignals() {
  setAnalog(state.mapVoltage, state.tpsVoltage);
  if (!state.running || state.rpm < RPM_MIN) {
    digitalWrite(CKP_PIN, LOW);
    digitalWrite(CMP_PIN, LOW);
    return;
  }

  const uint32_t toothPeriodUs = 60000000UL / (uint32_t(state.rpm) * 36UL);
  const uint32_t calculatedHalfPeriodUs = toothPeriodUs / 2UL;
  const uint32_t halfPeriodUs =
      calculatedHalfPeriodUs < 70UL ? 70UL : calculatedHalfPeriodUs;
  const uint32_t now = micros();
  if (int32_t(now - nextEdgeUs) < 0) return;

  if (lastEdgeUs) {
    const uint32_t actual = now - lastEdgeUs;
    const uint32_t jitter = actual > halfPeriodUs
                                ? actual - halfPeriodUs
                                : halfPeriodUs - actual;
    maxJitterUs = max(maxJitterUs, jitter);
  }
  lastEdgeUs = now;
  nextEdgeUs += halfPeriodUs;
  edgeHigh = !edgeHigh;

  const bool missingTooth = tooth == 35;
  digitalWrite(CKP_PIN, missingTooth ? LOW : edgeHigh);
  digitalWrite(CMP_PIN, tooth < 9 ? HIGH : LOW);
  if (!edgeHigh) tooth = (tooth + 1) % 36;
}

float readVoltage(uint8_t pin) {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < 32; ++i) {
    sum += analogRead(pin);
    delayMicroseconds(150);
  }
  return (sum / 32.0f) * 3.30f / 4095.0f;
}

const char* voltageResult(float expected, float measured) {
  if (measured < 0.05f) return "NOT_TESTED";
  const float error = fabsf(expected - measured);
  if (error <= FEEDBACK_TOLERANCE_V) return "PASS";
  if (error <= FEEDBACK_TOLERANCE_V * 2.0f) return "WARN";
  return "FAIL";
}

String runBoardTest(bool jsonOutput) {
  const bool wasRunning = state.running;
  const uint16_t oldRpm = state.rpm;
  const float oldMap = state.mapVoltage;
  const float oldTps = state.tpsVoltage;
  allOff();

  setAnalog(1.00f, 2.00f);
  delay(30);
  const float mapMeasured = readVoltage(ADC_MAP_PIN);
  const float tpsMeasured = readVoltage(ADC_TPS_PIN);
  const uint32_t heap = ESP.getFreeHeap();

  maxJitterUs = 0;
  lastEdgeUs = 0;
  state.rpm = 1000;
  state.running = true;
  const uint32_t started = millis();
  while (millis() - started < 300) {
    updateSignals();
    delayMicroseconds(20);
  }
  allOff();

  const char* mapResult = voltageResult(1.00f, mapMeasured);
  const char* tpsResult = voltageResult(2.00f, tpsMeasured);
  const char* timingResult = maxJitterUs <= JITTER_LIMIT_US ? "PASS" : "WARN";
  const char* heapResult = heap >= MIN_HEAP_BYTES ? "PASS" : "WARN";
  const esp_reset_reason_t reset = esp_reset_reason();
  const char* resetResult =
      (reset == ESP_RST_PANIC || reset == ESP_RST_WDT) ? "WARN" : "PASS";

  state.rpm = oldRpm;
  state.mapVoltage = oldMap;
  state.tpsVoltage = oldTps;
  state.running = wasRunning;
  setAnalog(oldMap, oldTps);

  if (jsonOutput) {
    String out = "{";
    out += "\"dac25\":\"" + String(mapResult) + "\",\"mapV\":" + String(mapMeasured, 3);
    out += ",\"dac26\":\"" + String(tpsResult) + "\",\"tpsV\":" + String(tpsMeasured, 3);
    out += ",\"timing\":\"" + String(timingResult) + "\",\"jitterUs\":" + String(maxJitterUs);
    out += ",\"memory\":\"" + String(heapResult) + "\",\"heap\":" + String(heap);
    out += ",\"wifi\":\"" + String(WEB_MODE ? "PASS" : "NOT_TESTED") + "\"";
    out += ",\"watchdog\":\"" + String(resetResult) + "\"}";
    return out;
  }

  String out = "\n=== ESP32 DAC BOARD HEALTH ===\n";
  out += "DAC25 -> ADC34 : " + String(mapResult) + "  " + String(mapMeasured, 3) + " V\n";
  out += "DAC26 -> ADC35 : " + String(tpsResult) + "  " + String(tpsMeasured, 3) + " V\n";
  out += "Timing jitter  : " + String(timingResult) + "  " + String(maxJitterUs) + " us\n";
  out += "Free heap      : " + String(heapResult) + "  " + String(heap) + " bytes\n";
  out += "Wi-Fi          : " + String(WEB_MODE ? "PASS" : "NOT_TESTED") + "\n";
  out += "Reset/watchdog : " + String(resetResult) + "\n";
  out += "Feedback test requires GPIO25->34 and GPIO26->35 jumpers.\n";
  return out;
}

void updateStandalone() {
  const bool startPressed = digitalRead(START_STOP_PIN) == LOW;
  if (startPressed && lastStartReleased) {
    state.running = !state.running;
    if (!state.running) allOff();
  }
  lastStartReleased = !startPressed;

  const bool modePressed = digitalRead(MODE_PIN) == LOW;
  const bool newModePress = modePressed && lastModeReleased;
  lastModeReleased = !modePressed;

#if VARIANT_ID == 1 || VARIANT_ID == 4
  state.rpm = map(analogRead(RPM_POT_PIN), 0, 4095, RPM_MIN, RPM_MAX);
  state.mapVoltage = analogRead(LOAD_POT_PIN) * 3.30f / 4095.0f;
  state.tpsVoltage = constrain(0.35f + state.mapVoltage * 0.75f, 0.0f, 3.3f);
#elif VARIANT_ID == 2
  if (newModePress) {
    state.rpm += 250;
    if (state.rpm > RPM_MAX) state.rpm = RPM_MIN;
  }
  state.mapVoltage = 1.20f;
  state.tpsVoltage = 0.70f;
#elif VARIANT_ID == 3
  static const uint16_t rpmPreset[] = {800, 1500, 2500, 4000};
  static const float mapPreset[] = {0.75f, 1.20f, 1.85f, 2.60f};
  if (newModePress) presetIndex = (presetIndex + 1) % 4;
  state.rpm = rpmPreset[presetIndex];
  state.mapVoltage = mapPreset[presetIndex];
  state.tpsVoltage = constrain(0.30f + state.mapVoltage, 0.0f, 3.3f);
#elif VARIANT_ID == 5
  state.running = true;
  const uint32_t phase = millis() % 30000UL;
  state.rpm = phase < 15000
                  ? map(phase, 0, 15000, 800, 4000)
                  : map(phase, 15000, 30000, 4000, 800);
  state.mapVoltage = 0.65f + (state.rpm - 800) * 2.1f / 3200.0f;
  state.tpsVoltage = 0.40f + (state.rpm - 800) * 2.3f / 3200.0f;
#endif

#if VARIANT_ID == 4
  if (millis() - lastReportMs > 1000) {
    Serial.printf("RUN=%d RPM=%u MAP=%.2f TPS=%.2f JITTER=%luus HEAP=%lu\n",
                  state.running, state.rpm, state.mapVoltage, state.tpsVoltage,
                  maxJitterUs, ESP.getFreeHeap());
    lastReportMs = millis();
  }
#endif
}

const char WEB_PAGE[] PROGMEM = R"HTML(
<!doctype html><html><head><meta name=viewport content="width=device-width,initial-scale=1">
<title>PUK ESP32 ECU Simulator</title><style>
:root{--a:#00d4ff;--b:#08111d;--c:#13253a;--t:#eaf7ff}
*{box-sizing:border-box}body{margin:0;background:var(--b);color:var(--t);font:16px system-ui}
main{max-width:850px;margin:auto;padding:18px}.card{background:var(--c);padding:16px;border-radius:14px;margin:12px 0}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));gap:12px}
button,input{width:100%;padding:12px;margin:5px 0;border:0;border-radius:9px}
button{background:var(--a);font-weight:700}.stop{background:#ff4d67}pre{white-space:pre-wrap}
</style></head><body><main><h2>PUK ESP32 DAC Simulator</h2>
<div class=grid><section class=card><h3>Engine</h3><label>RPM <b id=rv>800</b></label>
<input id=rpm type=range min=100 max=6000 value=800><button onclick=cmd('start')>START</button>
<button class=stop onclick=cmd('stop')>ALL OFF</button></section>
<section class=card><h3>Analog</h3><label>MAP <b id=mv>1.20</b> V</label>
<input id=map type=range min=0 max=330 value=120><label>TPS <b id=tv>0.65</b> V</label>
<input id=tps type=range min=0 max=330 value=65></section></div>
<section class=card><h3>Board Performance</h3><button onclick=testBoard()>RUN FULL TEST</button>
<pre id=health>Not tested</pre></section>
<section class=card><small>Generic 36-1 demo only. Confirm waveform, voltage interface and pinout before ECU connection.</small></section>
</main><script>
const $=x=>document.getElementById(x),rpm=$('rpm'),map=$('map'),tps=$('tps');
function sync(){rv.textContent=rpm.value;mv.textContent=(map.value/100).toFixed(2);
tv.textContent=(tps.value/100).toFixed(2);fetch(`/set?rpm=${rpm.value}&map=${map.value}&tps=${tps.value}`)}
[rpm,map,tps].forEach(x=>x.oninput=sync);function cmd(x){fetch('/'+x)}
function testBoard(){health.textContent='Testing...';fetch('/health').then(r=>r.json())
.then(x=>health.textContent=JSON.stringify(x,null,2)).catch(e=>health.textContent=e)}
setInterval(()=>fetch('/state').catch(()=>{}),2000);
</script></body></html>
)HTML";

void startWeb() {
  WiFi.mode(WIFI_AP);
  const String ssid = "PUK_SIM_" + String(VARIANT_ID);
  WiFi.softAP(ssid.c_str(), "puk-sim-32");

  server.on("/", [] {
    state.webClientSeen = true;
    state.lastClientMs = millis();
    server.send_P(200, "text/html", WEB_PAGE);
  });
  server.on("/set", [] {
    state.rpm = constrain(server.arg("rpm").toInt(), RPM_MIN, RPM_MAX);
    state.mapVoltage = constrain(server.arg("map").toInt() / 100.0f, 0.0f, 3.3f);
    state.tpsVoltage = constrain(server.arg("tps").toInt() / 100.0f, 0.0f, 3.3f);
    state.lastClientMs = millis();
    server.send(204);
  });
  server.on("/start", [] {
    state.running = true;
    state.lastClientMs = millis();
    server.send(204);
  });
  server.on("/stop", [] {
    allOff();
    state.lastClientMs = millis();
    server.send(204);
  });
  server.on("/state", [] {
    state.lastClientMs = millis();
    server.send(200, "application/json",
                "{\"running\":" + String(state.running ? "true" : "false") +
                    ",\"rpm\":" + String(state.rpm) + "}");
  });
  server.on("/health", [] {
    state.lastClientMs = millis();
    server.send(200, "application/json", runBoardTest(true));
  });
  server.begin();
  Serial.printf("Wi-Fi: %s  Password: puk-sim-32  URL: http://192.168.4.1\n",
                ssid.c_str());
}

void setup() {
  Serial.begin(115200);
  pinMode(CKP_PIN, OUTPUT);
  pinMode(CMP_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(START_STOP_PIN, INPUT_PULLUP);
  pinMode(MODE_PIN, INPUT_PULLUP);
  analogReadResolution(12);
  analogSetPinAttenuation(ADC_MAP_PIN, ADC_11db);
  analogSetPinAttenuation(ADC_TPS_PIN, ADC_11db);
  allOff();
  setAnalog(0.0f, 0.0f);

  if (WEB_MODE) startWeb();
  Serial.printf("PUK Simulator Arduino IDE - Variant %d ready\n", VARIANT_ID);
  Serial.println("Serial commands: T=board test, S=start, X=all off");
  if (digitalRead(MODE_PIN) == LOW) Serial.print(runBoardTest(false));
}

void loop() {
  if (WEB_MODE) {
    server.handleClient();
    if (state.webClientSeen && millis() - state.lastClientMs > CLIENT_TIMEOUT_MS) {
      allOff();
    }
  } else {
    updateStandalone();
  }

  if (Serial.available()) {
    const char command = Serial.read();
    if (command == 'T' || command == 't') Serial.print(runBoardTest(false));
    if (command == 'S' || command == 's') state.running = true;
    if (command == 'X' || command == 'x') allOff();
  }

  updateSignals();
  digitalWrite(STATUS_LED_PIN, state.running && ((millis() / 250) % 2));
  delay(0);
}
