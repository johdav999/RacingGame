// Matched real-build comparisons: no draw suppression or shader substitution.
// Serve the unchanged site at BEFORE_URL and the candidate at AFTER_URL.
const assert=require('node:assert/strict');
const fs=require('node:fs/promises');
const crypto=require('node:crypto');
const {chromium}=require('playwright');
const urls={before:process.env.BEFORE_URL||'http://127.0.0.1:8190',after:process.env.AFTER_URL||'http://127.0.0.1:8189'};
const out='build/evidence/performance-fix';
const mean=a=>a.reduce((s,x)=>s+x,0)/Math.max(a.length,1);
function summarize(times){const sorted=[...times].sort((a,b)=>a-b);return {frames:times.length,fps:1000/mean(times),medianMs:sorted[Math.floor(sorted.length*.5)],p95Ms:sorted[Math.floor(sorted.length*.95)],meanMs:mean(times)};}
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:process.env.BROWSER_EXECUTABLE||'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 const report={browser:await browser.version(),viewport:{width:2223,height:966},urls,backends:{}};
 let active;
 try{for(const backend of ['webgpu','webgl2']){
  const variants={};report.backends[backend]=variants;
  // Counterbalance ordering to reduce systematic warmup/thermal bias.
  for(const variant of backend==='webgpu'?['before','after']:['after','before']){
   const context=await browser.newContext({viewport:report.viewport,deviceScaleFactor:1});
   const page=await context.newPage(),errors=[];active=page;
   page.on('pageerror',e=>errors.push(String(e)));
   page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
   await page.goto(`${urls[variant]}/?renderer=${backend}&streetReview=0&plazaReview=0&cityReview=0`);
   await page.waitForFunction(()=>window.racingReady||window.racingCarError,null,{timeout:180000});
   assert(await page.evaluate(()=>!!window.racingReady),await page.evaluate(()=>window.racingCarError||'Not ready'));
   assert.equal(await page.evaluate(()=>racingBackend),backend);
   const identity=await page.evaluate(async()=>{
    const a=await navigator.gpu?.requestAdapter();const i=a?.info;
    const canvas=document.getElementById('viewport'),gl=racingBackend==='webgl2'?canvas.getContext('webgl2'):null,ext=gl?.getExtension('WEBGL_debug_renderer_info');
    return {gpu:i&&{vendor:i.vendor,architecture:i.architecture,device:i.device,description:i.description},glRenderer:ext?gl.getParameter(ext.UNMASKED_RENDERER_WEBGL):null,canvas:[canvas.width,canvas.height],build:await(await fetch('build-info.json')).json()};
   });
   assert(!identity.build.engineDirty&&!identity.build.developmentEngine);
   const wasmHash=crypto.createHash('sha256').update(await(await page.request.get(`${urls[variant]}/racing.wasm`)).body()).digest('hex');
   await page.evaluate(()=>{
    window.perfFrames=[];window.perfCounters=[];window.perfRecording=false;
    const original=window.enginePerformance;
    window.enginePerformance=s=>{original(s);if(perfRecording)perfCounters.push(s.current);};
    let last=0;function frame(now){if(perfRecording&&last)perfFrames.push(now-last);last=now;requestAnimationFrame(frame);}requestAnimationFrame(frame);
    window.setPerfView=(kind,id)=>{racingStreetReview=racingPlazaReview=racingCityReview=-1;if(kind==='street')racingStreetReview=id;if(kind==='plaza')racingPlazaReview=id;if(kind==='city')racingCityReview=id;};
   });
   const poses={};
   for(const [name,kind,id] of [['underpass','street',0],['shops','street',2],['bollards','city',4],['plaza','plaza',0],['paving-close','plaza',2]]){
    await page.evaluate(({kind,id})=>setPerfView(kind,id),{kind,id});
    await page.waitForTimeout(2000);
    await page.evaluate(()=>{perfFrames=[];perfCounters=[];perfRecording=true;});
    await page.waitForTimeout(6000);
    const samples=await page.evaluate(()=>{perfRecording=false;return {frames:perfFrames,counters:perfCounters};});
    poses[name]={...summarize(samples.frames),counters:samples.counters.at(-1)};
    await page.screenshot({path:`${out}/${backend}-${variant}-${name}.png`});
   }
   await page.evaluate(()=>{setPerfView('race',0);document.body.classList.remove('street-review');document.querySelector('.street-tour')?.remove();});
   await page.locator('#hover').click();await page.locator('#start').click();
   await page.waitForFunction(()=>racingState.phase===2);
   await page.evaluate(()=>{
    window.perfPilot=setInterval(()=>{
     const s=racingState;if(s.phase!==2||s.paused)return;
     let best=Infinity,index=0;
     for(let i=0;i<racingTrack.length;i++){const p=racingTrack[i],d=(p[0]-s.x)**2+(p[1]-s.z)**2;if(d<best){best=d;index=i;}}
     const t=racingTrack[(index+6)%racingTrack.length];let a=Math.atan2(t[0]-s.x,t[1]-s.z)-s.heading;a=Math.atan2(Math.sin(a),Math.cos(a));
     Module._race_input(101,900,Math.max(-1,Math.min(1,-a*2)));Module._race_input(100,901,s.speed<65?1:.1);Module._race_input(102,902,s.speed>76?.5:0);
    },16);
   });
   // First full lap warms every model/pipeline on the route. Measure lap two.
   await page.waitForFunction(()=>racingState.lap>=1,null,{timeout:180000});
   await page.evaluate(()=>{perfFrames=[];perfCounters=[];perfRecording=true;});
   await page.waitForFunction(()=>racingState.lap>=2,null,{timeout:180000});
   const race=await page.evaluate(()=>{perfRecording=false;clearInterval(perfPilot);Module._race_cancel();return {frames:perfFrames,counters:perfCounters,state:racingState};});
   assert.equal(race.state.recoveries,0);assert(race.state.valid&&race.state.best>0);
   assert.deepEqual(errors,[]);
   const frames=summarize(race.frames),counters=race.counters;
   const aggregate={triangles:mean(counters.map(c=>c.triangles)),drawCalls:mean(counters.map(c=>c.drawCalls)),lodParts:mean(counters.map(c=>c.cameraLodParts)),maxPasses:Math.max(...counters.map(c=>c.passes)),gpuTextureBytes:counters.at(-1)?.gpuTextureBytes};
   if(variant==='after'){assert(aggregate.lodParts>0,'No LODs selected during gameplay');assert(aggregate.maxPasses<=3,'Unexpected reflection capture pass');}
   variants[variant]={identity,wasmHash,poses,race:{...frames,aggregate,lapTime:race.state.last,state:race.state},errors};
   await page.screenshot({path:`${out}/${backend}-${variant}-race.png`});
   await fs.writeFile(`${out}/results.json`,JSON.stringify(report,null,2));
   console.log(backend,variant,JSON.stringify({race:frames,aggregate}));
   await context.close();
  }
  assert.deepEqual(variants.before.identity.gpu,variants.after.identity.gpu);
  assert.deepEqual(variants.before.identity.canvas,variants.after.identity.canvas);
  const before=variants.before.race,after=variants.after.race;
  variants.improvement={fpsRatio:after.fps/before.fps,triangleReduction:1-after.aggregate.triangles/before.aggregate.triangles,p95Reduction:1-after.p95Ms/before.p95Ms};
  assert(variants.improvement.fpsRatio>1.1,'FPS improvement below 10%');
  assert(variants.improvement.triangleReduction>.4,'Triangle reduction below 40%');
  await fs.writeFile(`${out}/results.json`,JSON.stringify(report,null,2));
  console.log(backend,'comparison passed',variants.improvement);
 }}catch(e){report.failure=String(e.stack||e);if(active&&!active.isClosed())await active.screenshot({path:`${out}/failure.png`}).catch(()=>{});throw e;}
 finally{await fs.writeFile(`${out}/results.json`,JSON.stringify(report,null,2));await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
