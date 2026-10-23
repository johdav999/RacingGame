'use strict';
// Ten seconds of completed game callbacks, bounded even on high-refresh displays.
class RacingFrameTimes {
  constructor() { this.reset(); }
  reset() { this.samples=[]; this.previous=null; }
  record(now,cpu,active=true) {
    if(!active || !Number.isFinite(now) || !Number.isFinite(cpu) || cpu<0) { this.reset(); return; }
    if(this.previous!==null && now>this.previous) this.samples.push({now,ms:now-this.previous,cpu});
    this.previous=now;
    while(this.samples.length && (this.samples[0].now<now-10000 || this.samples.length>2400)) this.samples.shift();
  }
  summary() {
    const samples=this.samples, n=samples.length;
    if(!n)return null;
    const sorted=samples.map(s=>s.ms).sort((a,b)=>a-b);
    const mean=samples.reduce((a,s)=>a+s.ms,0)/n;
    return {frames:n,seconds:samples.reduce((a,s)=>a+s.ms,0)/1000,fps:1000/mean,meanMs:mean,
      p95Ms:sorted[Math.ceil(n*.95)-1],worstMs:sorted[n-1],
      slowFrames:samples.filter(s=>s.ms>33.34).length,
      cpuMs:samples.reduce((a,s)=>a+s.cpu,0)/n};
  }
}
if(typeof module!=='undefined')module.exports=RacingFrameTimes;
