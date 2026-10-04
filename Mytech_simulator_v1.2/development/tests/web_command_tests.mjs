import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';

// Execute the actual embedded UI script with mocked DOM/network, without a board.
const page = fs.readFileSync(new URL('../ESP32_WiFi_StandAlone_Test/WebPage.h', import.meta.url), 'utf8');
const script = page.match(/<script>([\s\S]*?)<\/script>/)[1];
const elements = new Map();
const document = {
  activeElement: null,
  addEventListener(){},
  getElementById(id) {
    if (!elements.has(id)) elements.set(id, {id, value: 0, textContent: '', innerHTML: '', style: {}, classList: {toggle(){}}});
    return elements.get(id);
  }
};
const state = {version:'test', running:false, rpm:1000, profile:0, ckp:true, cmp:true, led:false, scope:false, tps:10, map:40, ect:50, iat:40, o2:50};
const calls = [];
let nextCommand = {ok:true, data:state};
const context = vm.createContext({document, setInterval(){}, async fetch(url, options={}) {
  calls.push({url, options});
  if (url === '/api/set') {
    if (nextCommand instanceof Error) throw nextCommand;
    return {ok:nextCommand.ok, status:nextCommand.ok ? 200 : 400, async json(){return nextCommand.data;}};
  }
  return {ok:true, status:200, async json(){return url === '/api/profiles' ? [] : state;}};
}});
vm.runInContext(script, context);
await new Promise(resolve => setImmediate(resolve));
for (const [key, value] of [['run',1],['rpm',0],['tps',100],['selftest',1]]) {
  assert.equal(await context.cmd(key,value), true);
  const call = calls.filter(x => x.url === '/api/set').at(-1);
  assert.equal(call.options.method, 'POST');
  assert.equal(call.options.headers['Content-Type'], 'text/plain');
  assert.equal(call.options.body, `${key}=${value}`);
  assert.equal(call.url.includes('?'), false);
}
nextCommand = {ok:false, data:{error:'invalid integer'}};
assert.equal(await context.cmd('rpm','bad'), false);
await new Promise(resolve => setImmediate(resolve));
assert.match(document.getElementById('commandError').textContent, /invalid integer/);
assert.equal(document.getElementById('status').textContent, 'STOPPED');
assert.equal(document.getElementById('rpm').value, 1000);
assert.ok(calls.some(x => x.url === '/api/state' && !x.options.method));
nextCommand = new Error('network unavailable');
assert.equal(await context.cmd('run',1), false);
assert.match(document.getElementById('commandError').textContent, /network unavailable/);
nextCommand = {ok:true, data:state};
assert.equal(await context.cmd('run',0), true);
assert.equal(document.getElementById('commandError').textContent, '');
console.log('Embedded UI command tests passed: POST body, rejection, network failure, and subsequent success.');
