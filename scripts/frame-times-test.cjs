const assert=require('node:assert/strict');
const Frames=require('../web/frame-times.js');
const f=new Frames();
f.record(0,2);assert.equal(f.summary(),null);
for(let i=1;i<=20;i++)f.record(i*10,2);
f.record(400,4);
let s=f.summary();assert.equal(s.frames,21);assert.equal(s.p95Ms,10);assert.equal(s.worstMs,200);assert.equal(s.slowFrames,1);assert.equal(s.fps,52.5);
// A long visible hitch must be retained, while hidden time must not be counted.
f.record(3400,2);assert.equal(f.summary().worstMs,3000);
f.record(10000,0,false);assert.equal(f.summary(),null);
f.record(20000,2);f.record(20010,2);assert.equal(f.summary().meanMs,10);
for(let i=1;i<=1200;i++)f.record(20010+i*10,1);
assert.equal(f.summary().worstMs,10);assert(f.samples.length<=1001);
f.reset();for(let i=0;i<3000;i++)f.record(i,1);assert.equal(f.samples.length,2400);
console.log('Frame-time statistics, stutters, lifecycle reset and bounds passed');
