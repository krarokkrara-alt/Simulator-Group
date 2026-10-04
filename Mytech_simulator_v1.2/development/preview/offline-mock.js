/* Offline demonstration only. This is not the firmware parser or an HTTP test. */
(() => {
  const config = window.__offlinePreviewConfig;
  let selfTestTimer;
  const state = {version:config.version+' / OFFLINE MOCK', running:false, rpm:1000, ckp:true, cmp:true, led:false, scope:false, selfTest:false, profile:0, tps:10, map:40, ect:50, iat:40, o2:50};
  const booleans = new Set(['run','ckp','cmp','led','scope','selftest']);
  const allowed = new Set(['run','rpm','ckp','cmp','led','scope','profile','selftest','tps','map','ect','iat','o2']);
  const clone = value => JSON.parse(JSON.stringify(value));
  const reply = (status,data) => Promise.resolve({ok:status>=200&&status<300,status,json:async()=>clone(data),text:async()=>JSON.stringify(data)});
  // Every fetch stays in this mock; there is deliberately no native-fetch fallback.
  window.fetch = (input,options={}) => {
    const path = String(input).split('?')[0];
    const method = (options.method||'GET').toUpperCase();
    if (path==='/api/state' && method==='GET') return reply(200,state);
    if (path==='/api/profiles' && method==='GET') return reply(200,config.profiles);
    if (path!=='/api/set') return reply(404,{error:'Offline preview endpoint only'});
    if (method!=='POST') return reply(405,{error:'Use POST with text/plain command body (offline mock)'});
    const headers = new Headers(options.headers||{});
    if ((headers.get('Content-Type')||'').split(';')[0].trim()!=='text/plain') return reply(415,{error:'Content-Type must be text/plain (offline mock)'});
    const body = options.body;
    if (typeof body!=='string' || body.length===0 || new TextEncoder().encode(body).length>256 || /[^\x21-\x7e]/.test(body)) return reply(400,{error:'Invalid ASCII command body (offline mock)'});
    const staged = new Map();
    for (const segment of body.split('&')) {
      const pair = segment.match(/^([a-z]+)=([0-9]+)$/);
      if (!pair || !allowed.has(pair[1]) || staged.has(pair[1])) return reply(400,{error:'Malformed, unknown or duplicate command (offline mock)'});
      const [,key,text] = pair;
      if (booleans.has(key) && text!=='0' && text!=='1') return reply(400,{error:'Boolean must be 0 or 1 (offline mock)'});
      const value = Number(text);
      const maximum = key==='rpm'?config.maxRpm:key==='profile'?config.profiles.length-1:booleans.has(key)?1:100;
      if (!Number.isSafeInteger(value) || value>maximum) return reply(400,{error:'Command value out of range (offline mock)'});
      staged.set(key,value);
    }
    for (const [key,value] of staged) {
      if (key==='selftest') continue;
      state[key==='run'?'running':key] = booleans.has(key)?value===1:value;
    }
    if (staged.get('selftest')===1) {
      clearTimeout(selfTestTimer);
      state.selfTest=state.led=state.scope=true;
      selfTestTimer=setTimeout(()=>{state.selfTest=state.led=state.scope=false;},3000);
    }
    return reply(200,state);
  };
})();
