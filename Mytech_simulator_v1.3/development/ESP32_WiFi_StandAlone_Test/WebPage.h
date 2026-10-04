#pragma once

const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><meta http-equiv="Cache-Control" content="no-store, no-cache, must-revalidate"><meta http-equiv="Pragma" content="no-cache">
<title>ESP32 ECU Simulator</title><style>
:root{color-scheme:dark;font-family:system-ui}body{margin:auto;max-width:820px;padding:18px;background:#0b1220;color:#e5e7eb}
h1{font-size:1.5rem}.card{background:#172033;border:1px solid #334155;border-radius:14px;padding:16px;margin:12px 0}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(210px,1fr));gap:12px}button{padding:12px 18px;border:0;border-radius:9px;font-weight:700;margin:4px;background:#334155;color:white}.start{background:#16a34a}.stop{background:#dc2626}.active{outline:3px solid #38bdf8}input{width:100%}.value{float:right;color:#38bdf8}#status{font-weight:700;color:#fbbf24}
select{width:100%;padding:11px;border-radius:8px;background:#0f172a;color:#e5e7eb;border:1px solid #475569}.info{line-height:1.55}.warning{color:#fbbf24;font-size:.9rem}.unverified{color:#f87171;font-weight:700}
</style></head><body><h1>ESP32 Wi-Fi Stand Alone Test <small id="version" style="font-size:.55em;color:#94a3b8"></small></h1><div class="card"><span id="status">Connecting...</span><br>
<button class="start" onclick="cmd('run',1)">START</button><button class="stop" onclick="cmd('run',0)">STOP</button><button onclick="cmd('selftest',1)">SELF TEST</button><p id="commandError" class="warning" role="alert"></p></div>
<div class="card"><h3>Vehicle / Trigger profile (CKP / CMP)</h3><div class="grid"><label>Brand<select id="brand" onchange="selectBrand(this.value)"></select></label><label>Model / Engine<select id="profile" onchange="selectProfile(this.value)"></select></label></div>
<div id="profileInfo" class="info"></div><svg id="wave" viewBox="0 0 740 90" style="width:100%;margin-top:8px" role="img" aria-label="CKP and CMP pattern over 720 degrees"></svg><p class="warning">Important: verify the exact model year, engine code and wiring diagram. ESP32 outputs are 3.3 V digital; Magnetic VR simulation requires an external bipolar VR driver/conditioner.</p></div>
<div class="card"><label>RPM <span class="value" id="rpmV"></span></label><input id="rpm" type="range" min="0" max="8000" step="50" oninput="show(this)" onchange="cmd('rpm',this.value)"></div>
<div class="grid"><div class="card"><h3>Engine signals</h3><button id="ckp" onclick="toggle('ckp')">CKP</button><button id="cmp" onclick="toggle('cmp')">CMP</button></div>
<div class="card"><h3>GPIO test</h3><button id="led" onclick="toggle('led')">LED GPIO 2</button><button id="scope" onclick="toggle('scope')">SCOPE GPIO 4</button></div></div>
<div class="card"><h3>Sensor outputs (PWM 0-100%)</h3><div id="sensors"></div></div>
<script>
let state={},profiles=[];const names=['tps','map','ect','iat','o2'];
const statusEl=document.getElementById('status'),versionEl=document.getElementById('version'),rpmEl=document.getElementById('rpm'),profileEl=document.getElementById('profile'),profileInfoEl=document.getElementById('profileInfo'),sensorsEl=document.getElementById('sensors');
const commandErrorEl=document.getElementById('commandError');
const activePointers=new Map(),keyboardRanges=new Set(),dirtyRanges=new Set(),rangeRevisions=new Map(),pendingCommands=new Map();
const pendingValues=new Map(),commandQueue=[],commandKeys=new Set(['run','rpm','ckp','cmp','led','scope','profile','selftest',...names]);
const maxQueuedCommands=commandKeys.size+1;
let inFlightCommand=null;
let commandSequence=0,refreshSequence=0;
function isRange(e){return e&&e.type==='range'&&(e.id==='rpm'||names.includes(e.id))}
function rangeBusy(e){return pendingCommands.has(e.id)||keyboardRanges.has(e.id)||[...activePointers.values()].includes(e.id)||dirtyRanges.has(e.id)}
document.addEventListener('pointerdown',e=>{if(isRange(e.target))activePointers.set(e.pointerId,e.target.id)},true);
['pointerup','pointercancel'].forEach(type=>document.addEventListener(type,e=>{const id=activePointers.get(e.pointerId);if(activePointers.delete(e.pointerId)){if(type==='pointercancel')dirtyRanges.delete(id);refresh()}},true));
document.addEventListener('input',e=>{if(isRange(e.target)){dirtyRanges.add(e.target.id);rangeRevisions.set(e.target.id,(rangeRevisions.get(e.target.id)||0)+1)}},true);
document.addEventListener('keydown',e=>{if(isRange(e.target)&&['ArrowLeft','ArrowRight','ArrowUp','ArrowDown','Home','End','PageUp','PageDown'].includes(e.key))keyboardRanges.add(e.target.id)},true);
document.addEventListener('keyup',e=>{if(isRange(e.target)&&keyboardRanges.delete(e.target.id))refresh()},true);
document.addEventListener('focusout',e=>{if(isRange(e.target)){dirtyRanges.delete(e.target.id);keyboardRanges.delete(e.target.id);for(const [pointer,id]of activePointers)if(id===e.target.id)activePointers.delete(pointer);refresh()}},true);
function finishCommand(k,sequence,editRevision){if(pendingCommands.get(k)===sequence){pendingCommands.delete(k);pendingValues.delete(k);if((rangeRevisions.get(k)||0)===editRevision)dirtyRanges.delete(k)}}
async function drainCommands(){
 if(inFlightCommand||commandQueue.length===0)return;
 const item=commandQueue.shift();inFlightCommand=item;let accepted=false;
 try{
  const r=await fetch('/api/set',{method:'POST',headers:{'Content-Type':'text/plain'},body:item.k+'='+item.v,cache:'no-store'});
  const data=await r.json();if(!r.ok)throw Error(data.error||('HTTP '+r.status));
  finishCommand(item.k,item.sequence,item.editRevision);commandErrorEl.textContent='';render(data);accepted=true;
 }catch(e){finishCommand(item.k,item.sequence,item.editRevision);commandErrorEl.textContent='Command failed ('+item.k+'): '+e.message;
 }finally{
  inFlightCommand=null;item.resolve(accepted);
  if(commandQueue.length)drainCommands();else refresh();
 }
}
function cmd(k,v){
 if(!commandKeys.has(k)){commandErrorEl.textContent='Unsupported UI command';return Promise.resolve(false)}
 const sequence=++commandSequence,editRevision=rangeRevisions.get(k)||0;pendingCommands.set(k,sequence);
 pendingValues.set(k,v);
 return new Promise(resolve=>{
  const item={k,v,sequence,editRevision,resolve},stop=k==='run'&&String(v)==='0';
  let replacementIndex=-1;
  for(let i=commandQueue.length-1;i>=0;--i){
   const queued=commandQueue[i];
   // A new START may replace queued START, but never remove a queued STOP.
   if(queued.k===k&&(k!=='run'||stop||String(queued.v)!=='0')){replacementIndex=i;commandQueue.splice(i,1);queued.resolve(false)}
  }
  if(commandQueue.length>=maxQueuedCommands){finishCommand(k,sequence,editRevision);commandErrorEl.textContent='Command queue is full';resolve(false);return}
  if(stop)commandQueue.unshift(item);else if(replacementIndex>=0)commandQueue.splice(replacementIndex,0,item);else commandQueue.push(item);
  drainCommands();
 });
}
const brandEl=document.getElementById('brand'),waveEl=document.getElementById('wave');
function esc(t){return String(t).replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]))}
function brands(){return [...new Set(profiles.map(p=>p.brand))]}
function fillModels(brand){profileEl.innerHTML=profiles.filter(p=>p.brand===brand).map(x=>`<option value="${x.id}">${esc(x.model)} / ${esc(x.engine)}</option>`).join('')}
let shownProfile=null;
// Switching while RUNNING restarts the CKP/CMP cycle at slot 0, so the ECU loses sync briefly.
function confirmSwitch(){return !state.running||typeof confirm!=='function'||confirm('Engine is RUNNING. Changing the vehicle restarts the CKP/CMP cycle. Continue?')}
function selectBrand(b){const first=profiles.find(p=>p.brand===b);if(!first)return;if(!confirmSwitch()){shownProfile=null;showProfile(state.profile);return}fillModels(b);profileEl.value=first.id;showProfile(first.id);cmd('profile',first.id)}
function selectProfile(v){if(!confirmSwitch()){shownProfile=null;showProfile(state.profile);return}showProfile(v);cmd('profile',v)}
function wavePath(levels,y0){const w=720/levels.length;let d=`M10 ${levels[0]?y0-22:y0}`;levels.forEach((h,i)=>{const y=h?y0-22:y0;d+=` V${y} H${10+(i+1)*w}`});return d}
function drawWave(p){if(!waveEl||!p.teeth){if(waveEl)waveEl.innerHTML='';return}
 const slots=4*p.teeth,ckp=[],cmp=new Array(slots).fill(0);
 for(let s=0;s<slots;++s)ckp.push(s%2===0&&Math.floor(s/2)%p.teeth<p.teeth-p.missing);
 (p.cams||[]).forEach(([a,w])=>{const st=Math.floor(a*p.teeth/180),n=Math.floor(w*p.teeth/180);for(let k=0;k<n;++k)cmp[(st+k)%slots]=1});
 waveEl.innerHTML=`<text x="10" y="10" fill="#94a3b8" font-size="10">CKP ${esc(p.ckpPattern)}</text><path d="${wavePath(ckp,38)}" fill="none" stroke="#38bdf8" stroke-width="1"/><text x="10" y="52" fill="#94a3b8" font-size="10">CMP ${(p.cams||[]).length} pulse/720°</text><path d="${wavePath(cmp,82)}" fill="none" stroke="#4ade80" stroke-width="1.5"/><text x="370" y="10" fill="#64748b" font-size="10">360°</text><line x1="370" y1="12" x2="370" y2="86" stroke="#334155" stroke-dasharray="3 3"/>`}
function showProfile(id){const p=profiles.find(x=>x.id==id);if(!p)return;if(brandEl.value!==p.brand){brandEl.value=p.brand;fillModels(p.brand)}profileEl.value=p.id;
 if(shownProfile===p.id)return;shownProfile=p.id;
 profileInfoEl.innerHTML=`<p><b>${esc(p.brand)} ${esc(p.model)}</b> — ${esc(p.engine)}</p><b>CKP:</b> ${esc(p.ckpType)} — ${esc(p.ckpPattern)}<br><b>CMP:</b> ${esc(p.cmpType)} — ${(p.cams||[]).length} pulse/720°`+
 (p.verified===false?`<br><span class="unverified">UNVERIFIED tooth data — check the service manual before connecting to an ECU</span>`:'')+`<br><small>${esc(p.notes)}</small>`+(p.source?`<br><small>Source: ${esc(p.source)}</small>`:'');drawWave(p)}
function toggle(k){cmd(k,pendingValues.has(k)?Number(pendingValues.get(k))===1?0:1:state[k]?0:1)}
function show(e){document.getElementById(e.id+'V').textContent=e.value+(e.id==='rpm'?' rpm':'%')}
function render(s){state=s;versionEl.textContent='v'+s.version;statusEl.textContent=s.running?'RUNNING':'STOPPED';statusEl.style.color=s.running?'#4ade80':'#f87171';
 ['ckp','cmp','led','scope'].forEach(k=>document.getElementById(k).classList.toggle('active',s[k]));
 if(!rangeBusy(rpmEl))rpmEl.value=s.rpm;show(rpmEl);if(!pendingCommands.has('profile'))showProfile(s.profile);names.forEach(k=>{let e=document.getElementById(k);if(!rangeBusy(e))e.value=s[k];show(e)});}
function refresh(){const poll=++refreshSequence,revision=commandSequence,startedPending=pendingCommands.size>0||inFlightCommand!==null||commandQueue.length>0;return fetch('/api/state',{cache:'no-store'}).then(r=>{if(!r.ok)throw Error(r.status);return r.json()}).then(s=>{if(!startedPending&&poll===refreshSequence&&revision===commandSequence&&pendingCommands.size===0)render(s)}).catch(()=>{if(!startedPending&&poll===refreshSequence&&revision===commandSequence&&pendingCommands.size===0)statusEl.textContent='Disconnected'})}
sensorsEl.innerHTML=names.map(k=>`<label>${k.toUpperCase()} <span class="value" id="${k}V"></span></label><input id="${k}" type="range" min="0" max="100" oninput="show(this)" onchange="cmd('${k}',this.value)">`).join('');
fetch('/api/profiles',{cache:'no-store'}).then(r=>r.json()).then(p=>{profiles=p;brandEl.innerHTML=brands().map(b=>`<option value="${esc(b)}">${esc(b)}</option>`).join('');if(p.length){brandEl.value=p[0].brand;fillModels(p[0].brand)}refresh()}).catch(()=>{statusEl.textContent='Disconnected';refresh()});
setInterval(refresh,1000);
</script></body></html>)HTML";
