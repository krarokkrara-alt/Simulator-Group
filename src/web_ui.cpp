#include "web_ui.h"
#include "config.h"

#if APP_MODE == 1
#include <WiFi.h>
#include <WebServer.h>

namespace {
SimulatorState* gState;
SignalEngine* gSignals;
BoardHealthMonitor* gMonitor;
BoardHealth gLastHealth;
WebServer gServer(80);

const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html><head><meta name=viewport content="width=device-width,initial-scale=1">
<title>PUK ESP32 ECU Simulator</title><style>
:root{--a:#00d4ff;--b:#08111d;--c:#13253a;--t:#eaf7ff}
*{box-sizing:border-box}body{margin:0;background:var(--b);color:var(--t);font:16px system-ui}
main{max-width:850px;margin:auto;padding:18px}.card{background:var(--c);padding:16px;border-radius:14px;margin:12px 0}
h1{font-size:23px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:12px}
button,input{width:100%;padding:12px;margin:5px 0;border-radius:9px;border:0}
button{background:var(--a);font-weight:700}.stop{background:#ff4d67}.metric{font-size:22px}
pre{white-space:pre-wrap;color:#aeeaff}.v7{--a:#78ff9c}.v8{--a:#ffb84d}.v9{--a:#b79cff}.v10{--a:#ff657a}
</style></head><body><main id=app><h1>PUK ESP32 DAC Simulator · Variant %VAR%</h1>
<div class=grid><section class=card><h3>Engine</h3><label>RPM <b id=rv>800</b></label>
<input id=rpm type=range min=100 max=6000 value=800>
<button onclick=cmd('start')>START</button><button class=stop onclick=cmd('stop')>ALL OFF</button></section>
<section class=card><h3>Analog Sensors</h3><label>MAP <b id=mv>1.20</b> V</label>
<input id=map type=range min=0 max=330 step=1 value=120>
<label>TPS <b id=tv>0.65</b> V</label><input id=tps type=range min=0 max=330 step=1 value=65></section></div>
<section class=card><h3>Board Performance / Self Test</h3>
<button onclick=testBoard()>RUN FULL TEST</button><pre id=health>Not tested</pre></section>
<section class=card><small>Generic 36-1 demo only. Confirm waveform and protected interface before connecting an ECU.</small></section>
</main><script>
document.body.className='v%VAR%';
const $=x=>document.getElementById(x);
function sync(){rv.textContent=rpm.value;mv.textContent=(map.value/100).toFixed(2);
tv.textContent=(tps.value/100).toFixed(2);fetch(`/set?rpm=${rpm.value}&map=${map.value}&tps=${tps.value}`)}
[rpm,map,tps].forEach(x=>x.oninput=sync);
function cmd(x){fetch('/'+x)}function testBoard(){health.textContent='Testing...';
fetch('/health').then(r=>r.json()).then(x=>health.textContent=JSON.stringify(x,null,2)).catch(e=>health.textContent=e)}
setInterval(()=>fetch('/state').then(r=>r.json()).then(()=>{}).catch(()=>{}),2000);
</script></body></html>)HTML";
}

void WebUi::begin(SimulatorState& state, SignalEngine& signals, BoardHealthMonitor& monitor) {
  gState = &state;
  gSignals = &signals;
  gMonitor = &monitor;
  WiFi.mode(WIFI_AP);
  const String ssid = "PUK_SIM_" + String(VARIANT_ID);
  WiFi.softAP(ssid.c_str(), Config::AP_PASSWORD);

  gServer.on("/", [] {
    String page = FPSTR(PAGE);
    page.replace("%VAR%", String(VARIANT_ID));
    gState->webClientSeen = true;
    gState->lastClientMs = millis();
    gServer.send(200, "text/html", page);
  });
  gServer.on("/set", [] {
    gState->rpm = constrain(gServer.arg("rpm").toInt(), Config::RPM_MIN, Config::RPM_MAX);
    gState->mapVoltage = constrain(gServer.arg("map").toInt() / 100.0f, 0.0f, 3.3f);
    gState->tpsVoltage = constrain(gServer.arg("tps").toInt() / 100.0f, 0.0f, 3.3f);
    gState->lastClientMs = millis();
    gServer.send(204);
  });
  gServer.on("/start", [] { gState->running = true; gServer.send(204); });
  gServer.on("/stop", [] { gState->running = false; gSignals->allOff(); gServer.send(204); });
  gServer.on("/state", [] {
    gState->lastClientMs = millis();
    String json = "{\"running\":" + String(gState->running ? "true" : "false") +
                  ",\"rpm\":" + String(gState->rpm) + "}";
    gServer.send(200, "application/json", json);
  });
  gServer.on("/health", [] {
    gLastHealth = gMonitor->runFullTest(*gSignals, *gState, true);
    gServer.send(200, "application/json", gMonitor->toJson(gLastHealth));
  });
  gServer.begin();
  server_ = &gServer;
}

void WebUi::loop() { gServer.handleClient(); }
int8_t WebUi::rssi() const { return WiFi.RSSI(); }

#else
void WebUi::begin(SimulatorState&, SignalEngine&, BoardHealthMonitor&) {}
void WebUi::loop() {}
int8_t WebUi::rssi() const { return 0; }
#endif

