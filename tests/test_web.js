// Exercise the shipped Web Bluetooth time command with a deterministic browser clock.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
let zone = '3', browserOffset = 0;
const now = 1792530000000; // 2026-10-20 21:00 UTC = Moscow midnight
class MockDate extends Date {
  constructor(...args) { super(...(args.length ? args : [now])); }
  static now() { return now; }
  getTimezoneOffset() { return browserOffset; }
}
const ctx = vm.createContext({
  document: {body: {}, getElementById: id => ({value: zone})},
  Date: MockDate, Uint8Array, console, confirm: () => true,
});
vm.runInContext(fs.readFileSync('html/js/main.js', 'utf8'),ctx);
vm.runInContext('globalThis.sent=[]; globalThis.logs=[]; write=async (cmd,data)=>{sent.push({cmd,data});return true;}; addLog=s=>logs.push(s);',ctx);
(async () => {
  await vm.runInContext('syncTime(1)',ctx);
  assert.equal(ctx.sent[0].cmd,0x20);
  const packet=Array.from(ctx.sent[0].data);
  assert.deepEqual(packet,[(now/1000>>>24)&255,(now/1000>>>16)&255,(now/1000>>>8)&255,(now/1000)&255,3,1]);
  assert.equal((now/1000+packet[4]*3600)%86400,0);
  zone='browser';browserOffset=300;
  await vm.runInContext('syncTime(1)',ctx);
  assert.equal(ctx.sent[1].data[4],251); // signed -5, upstream's int8_t protocol
  browserOffset=-330;
  await vm.runInContext('syncTime(1)',ctx);
  assert.equal(ctx.sent.length,2); // fractional offsets must not silently truncate
  assert.match(ctx.logs.at(-1),/целое число/);
  console.log('BLE sync packet: Moscow, signed offset and fractional-zone rejection passed');
})().catch(e=>{ console.error(e);process.exit(1); });
