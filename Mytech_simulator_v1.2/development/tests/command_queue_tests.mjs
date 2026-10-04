import assert from 'node:assert/strict';
import {setup,flush} from './ui_deferred_harness.mjs';
function idle(h){assert.equal(h.inspect('commandQueue.length'),0);assert.equal(h.inspect('pendingCommands.size'),0);assert.equal(h.inspect('pendingValues.size'),0);assert.equal(h.inspect('inFlightCommand'),null)}

// Unsent repeated slider values coalesce; no second POST until the first settles.
{
 const h=await setup();h.edit('rpm',2000);const first=h.context.cmd('rpm',2000);
 h.edit('rpm',2100);const replaced=h.context.cmd('rpm',2100);
 h.edit('rpm',2200);const latest=h.context.cmd('rpm',2200);
 assert.equal(await replaced,false);assert.equal(h.commands.length,1);assert.equal(h.inspect('commandQueue.length'),1);
 h.reply(0,{rpm:2000});assert.equal(await first,true);assert.equal(h.commands.length,2);assert.equal(h.commands[1].body,'rpm=2200');assert.equal(h.get('rpm').value,2200);
 h.reply(1,{rpm:2200});assert.equal(await latest,true);idle(h);
}
// Coalescing retains a field's position relative to other queued fields.
{
 const h=await setup();const current=h.context.cmd('tps',40),oldRpm=h.context.cmd('rpm',2000),map=h.context.cmd('map',50),latestRpm=h.context.cmd('rpm',2200);
 assert.equal(await oldRpm,false);h.reply(0,{tps:40});await current;assert.equal(h.commands[1].body,'rpm=2200');
 h.reply(1,{rpm:2200});await latestRpm;assert.equal(h.commands[2].body,'map=50');h.reply(2,{map:50});await map;idle(h);
}
// STOP replaces unsent START and jumps ahead of queued RPM/sensor commands.
{
 const h=await setup();const current=h.context.cmd('tps',40),rpm=h.context.cmd('rpm',2000),start=h.context.cmd('run',1),map=h.context.cmd('map',50),stop=h.context.cmd('run',0);
 assert.equal(await start,false);assert.equal(h.commands.length,1);
 h.reply(0,{tps:40});await current;assert.equal(h.commands[1].body,'run=0');
 h.reply(1,{running:false});await stop;assert.equal(h.commands[2].body,'rpm=2000');
 h.reply(2,{rpm:2000});await rpm;assert.equal(h.commands[3].body,'map=50');
 h.reply(3,{map:50});await map;idle(h);
}
// A later explicit START does not remove the queued STOP.
{
 const h=await setup();const current=h.context.cmd('rpm',2000),stop=h.context.cmd('run',0),start=h.context.cmd('run',1);
 assert.equal(h.commands.length,1);assert.equal(h.inspect('commandQueue.length'),2);
 h.reply(0,{rpm:2000});await current;assert.equal(h.commands[1].body,'run=0');
 h.reply(1,{running:false});await stop;assert.equal(h.commands[2].body,'run=1');assert.equal(h.get('status').textContent,'STOPPED');
 h.reply(2,{running:true});await start;idle(h);
}
// Network rejection releases queued STOP, without treating the failure as cancellation.
{
 const h=await setup();const current=h.context.cmd('rpm',2000),stop=h.context.cmd('run',0);
 h.commands[0].reject(new Error('network outcome unknown'));assert.equal(await current,false);assert.equal(h.commands.length,2);assert.equal(h.commands[1].body,'run=0');
 assert.match(h.get('commandError').textContent,/network outcome unknown/);
 h.reply(1,{running:false});assert.equal(await stop,true);idle(h);
}
// A hung in-flight fetch prevents later dispatch; STOP is not falsely acknowledged.
{
 const h=await setup();const current=h.context.cmd('rpm',2000),stop=h.context.cmd('run',0);let stopped=false;stop.then(()=>{stopped=true});
 await flush();assert.equal(h.commands.length,1);assert.equal(stopped,false);
 h.reply(0,{rpm:2000});await current;h.reply(1,{running:false});await stop;idle(h);
}
// Repeated unsent STOP promises settle when replaced, and all queue guards release.
{
 const h=await setup();const current=h.context.cmd('rpm',2000),oldStop=h.context.cmd('run',0),newStop=h.context.cmd('run',0);
 assert.equal(await oldStop,false);assert.equal(h.inspect('commandQueue.length'),1);
 h.reply(0,{rpm:2000});await current;h.reply(1,{running:false});await newStop;idle(h);
}
// Toggles follow pending intent, rather than repeating the last acknowledged value.
{
 const h=await setup();h.context.toggle('ckp');h.context.toggle('ckp');assert.equal(h.commands[0].body,'ckp=0');assert.equal(h.inspect('commandQueue[0].v'),1);
 h.reply(0,{ckp:false});await flush();assert.equal(h.commands[1].body,'ckp=1');h.reply(1,{ckp:true});await flush();idle(h);
}
// The known UI fields keep the unsent queue bounded under a large input burst.
{
 const h=await setup();const first=h.context.cmd('run',0),promises=[];
 for(let i=0;i<100;++i)for(const key of ['rpm','tps','map','ect','iat','o2'])promises.push(h.context.cmd(key,key==='rpm'?i*50:i));
 assert.equal(h.commands.length,1);assert.equal(h.inspect('commandQueue.length'),6);
 h.reply(0,{running:false});await first;
 for(let i=1;i<=6;++i){const [key,text]=h.commands[i].body.split('=');h.reply(i,{[key]:Number(text)});await flush();}
 await Promise.all(promises);idle(h);
}
// A queued profile choice is not overwritten by a different field's acknowledgement.
{
 const h=await setup();const current=h.context.cmd('tps',40);h.get('profile').value=1;const profile=h.context.cmd('profile',1);
 h.reply(0,{tps:40});await current;assert.equal(h.get('profile').value,1);
 h.reply(1,{profile:1});await profile;idle(h);
}
console.log('Command queue tests passed: 10 deferred ordering/coalescing/STOP/error/bounds/profile scenarios using actual UI code; no server cancellation or hardware claim.');
