import assert from 'node:assert/strict';
import {setup,flush,response} from './ui_deferred_harness.mjs';

// Touch pointer works even if the browser does not focus a range input.
{
 const h=await setup(),rpm=h.get('rpm'),tps=h.get('tps');
 h.emit('pointerdown',rpm,{pointerId:1,pointerType:'touch'});h.edit('rpm',2100);
 h.emit('pointerdown',tps,{pointerId:2,pointerType:'touch'});h.edit('tps',75);
 await h.context.refresh();assert.equal(rpm.value,2100);assert.equal(tps.value,75);
 h.emit('pointercancel',rpm,{pointerId:1});await flush();assert.equal(rpm.value,1000);assert.equal(tps.value,75);
 h.emit('pointercancel',tps,{pointerId:2});await flush();assert.equal(tps.value,10);
}
// Keyboard editing survives polling/key release; blur returns authoritative state.
{
 const h=await setup(),rpm=h.get('rpm');h.document.activeElement=rpm;
 h.emit('keydown',rpm,{key:'ArrowRight'});h.edit('rpm',3000);
 await h.context.refresh();assert.equal(rpm.value,3000);
 h.emit('keyup',rpm,{key:'ArrowRight'});await flush();assert.equal(rpm.value,3000);
 h.document.activeElement=null;h.emit('focusout',rpm);await flush();assert.equal(rpm.value,1000);
}
// A change command remains protected after pointer release until the POST completes.
{
 const h=await setup(),rpm=h.get('rpm');h.emit('pointerdown',rpm,{pointerId:3});h.edit('rpm',3200);
 const pending=h.context.cmd('rpm',rpm.value);h.emit('pointerup',rpm,{pointerId:3});await h.context.refresh();assert.equal(rpm.value,3200);
 h.reply(0,{rpm:3200});assert.equal(await pending,true);assert.equal(rpm.value,3200);
}
// A newer queued value remains protected while the older request settles first.
{
 const h=await setup();h.edit('rpm',2100);const old=h.context.cmd('rpm',2100);
 h.edit('rpm',2200);const latest=h.context.cmd('rpm',2200);
 assert.equal(h.commands.length,1);h.reply(0,{rpm:2100});await old;assert.equal(h.get('rpm').value,2200);
 assert.equal(h.commands.length,2);h.reply(1,{rpm:2200});await latest;await flush();assert.equal(h.get('rpm').value,2200);
}
// A state poll started before the command cannot overwrite its response.
{
 const h=await setup();h.holdPolls(true);const oldPoll=h.context.refresh();
 h.edit('rpm',2300);const command=h.context.cmd('rpm',2300);h.reply(0,{rpm:2300});await command;
 h.polls[0].resolve(response(h.polls[0].snapshot));await oldPoll;assert.equal(h.get('rpm').value,2300);
}
// A poll begun during a pending command stays ineligible after that command settles.
{
 const h=await setup();h.edit('rpm',2300);const command=h.context.cmd('rpm',2300);
 h.holdPolls(true);const pendingPoll=h.context.refresh();
 h.reply(0,{rpm:2300});await command;
 h.polls[0].resolve(response(h.polls[0].snapshot));await pendingPoll;assert.equal(h.get('rpm').value,2300);
}
// Failure releases the queue; the newer acknowledged success clears that error.
{
 const h=await setup();h.edit('rpm',2100);const old=h.context.cmd('rpm',2100);
 h.edit('rpm',2200);const latest=h.context.cmd('rpm',2200);
 h.reply(0,{error:'old error'},400);assert.equal(await old,false);assert.match(h.get('commandError').textContent,/old error/);
 h.reply(1,{rpm:2200});await latest;await flush();assert.equal(h.get('commandError').textContent,'');assert.equal(h.get('rpm').value,2200);
}
// Failed RPM rolls back while another sensor stays under the user's pointer.
{
 const h=await setup();h.edit('rpm',9000);const bad=h.context.cmd('rpm',9000);
 const tps=h.get('tps');h.emit('pointerdown',tps,{pointerId:7});h.edit('tps',55);
 h.reply(0,{error:'invalid RPM'},400);assert.equal(await bad,false);await flush();assert.equal(h.get('rpm').value,1000);assert.equal(tps.value,55);
 await h.context.refresh();assert.match(h.get('commandError').textContent,/invalid RPM/);assert.equal(tps.value,55);
}
// Completing an earlier POST must not clear a newer, still-uncommitted edit.
{
 const h=await setup();h.edit('rpm',2100);const old=h.context.cmd('rpm',2100);
 h.edit('rpm',2400);h.reply(0,{rpm:2100});await old;await h.context.refresh();assert.equal(h.get('rpm').value,2400);
 const latest=h.context.cmd('rpm',2400);h.reply(1,{rpm:2400});await latest;assert.equal(h.get('rpm').value,2400);
}
console.log('Active slider tests passed: 9 event/request-order scenarios using the actual embedded UI script. No browser, HTTP transport or board claim.');
