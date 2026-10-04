#pragma once

const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><meta http-equiv="Cache-Control" content="no-store, no-cache, must-revalidate"><meta http-equiv="Pragma" content="no-cache">
<title>ESP32 ECU Simulator</title><style>
:root{color-scheme:dark;font-family:system-ui}body{margin:auto;max-width:820px;padding:18px;background:#0b1220;color:#e5e7eb}
h1{font-size:1.5rem}.card{background:#172033;border:1px solid #334155;border-radius:14px;padding:16px;margin:12px 0}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(210px,1fr));gap:12px}button{padding:12px 18px;border:0;border-radius:9px;font-weight:700;margin:4px;background:#334155;color:white}.start{background:#16a34a}.stop{background:#dc2626}.active{outline:3px solid #38bdf8}input{width:100%}.value{float:right;color:#38bdf8}#status{font-weight:700;color:#fbbf24}
select{width:100%;padding:11px;border-radius:8px;background:#0f172a;color:#e5e7eb;border:1px solid #475569}.info{line-height:1.55}.warning{color:#fbbf24;font-size:.9rem}
</style></head><body><h1>ESP32 Wi-Fi Stand Alone Test <small id="version" style="font-size:.55em;color:#94a3b8"></small></h1><div class="card"><span id="status">Connecting...</span><br>
<button class="start" onclick="cmd('run',1)">START</button><button class="stop" onclick="cmd('run',0)">STOP</button><button onclick="cmd('selftest',1)">SELF TEST</button></div>
<div class="card"><h3>Vehicle / Trigger profile</h3><select id="profile" onchange="selectProfile(this.value)"></select>
<div id="profileInfo" class="info"></div><p class="warning">Important: verify the exact model year, engine code and wiring diagram. ESP32 outputs are 3.3 V digital; Magnetic VR simulation requires an external bipolar VR driver/conditioner.</p></div>
<div class="card"><label>RPM <span class="value" id="rpmV"></span></label><input id="rpm" type="range" min="0" max="8000" step="50" oninput="show(this)" onchange="cmd('rpm',this.value)"></div>
<div class="grid"><div class="card"><h3>Engine signals</h3><button id="ckp" onclick="toggle('ckp')">CKP</button><button id="cmp" onclick="toggle('cmp')">CMP</button></div>
<div class="card"><h3>GPIO test</h3><button id="led" onclick="toggle('led')">LED GPIO 2</button><button id="scope" onclick="toggle('scope')">SCOPE GPIO 4</button></div></div>
<div class="card"><h3>Sensor outputs (PWM 0-100%)</h3><div id="sensors"></div></div>
<script>
let state={},profiles=[];const names=['tps','map','ect','iat','o2'];
const statusEl=document.getElementById('status'),versionEl=document.getElementById('version'),rpmEl=document.getElementById('rpm'),profileEl=document.getElementById('profile'),profileInfoEl=document.getElementById('profileInfo'),sensorsEl=document.getElementById('sensors');
function cmd(k,v){fetch('/api/set?'+encodeURIComponent(k)+'='+encodeURIComponent(v),{cache:'no-store'}).then(refresh)}
function selectProfile(v){cmd('profile',v);showProfile(v)}
function showProfile(id){const p=profiles.find(x=>x.id==id);if(!p)return;profileInfoEl.innerHTML=`<p><b>${p.brand} ${p.model}</b> — ${p.engine}</p><b>CKP:</b> ${p.ckpType}<br><b>CMP:</b> ${p.cmpType}<br><b>Trigger:</b> ${p.ckpPattern}<br><small>${p.notes}</small>`}
function toggle(k){cmd(k,state[k]?0:1)}
function show(e){document.getElementById(e.id+'V').textContent=e.value+(e.id==='rpm'?' rpm':'%')}
function render(s){state=s;versionEl.textContent='v'+s.version;statusEl.textContent=s.running?'RUNNING':'STOPPED';statusEl.style.color=s.running?'#4ade80':'#f87171';
 ['ckp','cmp','led','scope'].forEach(k=>document.getElementById(k).classList.toggle('active',s[k]));
 rpmEl.value=s.rpm;show(rpmEl);profileEl.value=s.profile;showProfile(s.profile);names.forEach(k=>{let e=document.getElementById(k);if(document.activeElement!==e)e.value=s[k];show(e)});}
function refresh(){fetch('/api/state',{cache:'no-store'}).then(r=>{if(!r.ok)throw Error(r.status);return r.json()}).then(render).catch(()=>statusEl.textContent='Disconnected')}
sensorsEl.innerHTML=names.map(k=>`<label>${k.toUpperCase()} <span class="value" id="${k}V"></span></label><input id="${k}" type="range" min="0" max="100" oninput="show(this)" onchange="cmd('${k}',this.value)">`).join('');
fetch('/api/profiles',{cache:'no-store'}).then(r=>r.json()).then(p=>{profiles=p;profileEl.innerHTML=p.map(x=>`<option value="${x.id}">${x.brand} — ${x.model} / ${x.engine}</option>`).join('');refresh()}).catch(()=>{statusEl.textContent='Disconnected';refresh()});
setInterval(refresh,1000);
</script></body></html>)HTML";
