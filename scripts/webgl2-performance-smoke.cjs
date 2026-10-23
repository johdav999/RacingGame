const {chromium}=require('playwright');const fs=require('node:fs');const assert=require('node:assert/strict');const crypto=require('node:crypto');
const out=(process.env.EVIDENCE_DIR||'build/evidence/webgl2-material-fix/comparison');fs.mkdirSync(out,{recursive:true});
const urls={before:process.env.BEFORE_URL||'http://127.0.0.1:8190',after:process.env.AFTER_URL||'http://127.0.0.1:8197'};
function summary(a){const s=[...a].sort((a,b)=>a-b);return {fps:a.length*1000/a.reduce((a,b)=>a+b,0),medianMs:s[Math.floor(s.length*.5)],p95Ms:s[Math.floor(s.length*.95)],frames:a.length};}
(async()=>{
 const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files/Google/Chrome/Application/chrome.exe'}),report={browser:await browser.version(),viewport:{width:1440,height:900},runs:{}};
 try{for(const backend of ['webgl2','webgpu'])for(const variant of ['before','after']){
  const context=await browser.newContext({viewport:report.viewport,deviceScaleFactor:1}),page=await context.newPage(),errors=[];
  page.on('pageerror',e=>errors.push(String(e)));page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
  await page.addInitScript(()=>{
   window.programProbe={simpleLinks:0,generalLinks:0,simpleDraws:0,generalDraws:0,waterDraws:0};
   if(!window.WebGL2RenderingContext)return;
   const p=WebGL2RenderingContext.prototype,sources=new WeakMap(),attached=new WeakMap(),kinds=new WeakMap(),uniforms=new WeakMap(),water=new WeakMap();let current;
   const source=p.shaderSource;p.shaderSource=function(s,t){sources.set(s,t);return source.call(this,s,t);};
   const attach=p.attachShader;p.attachShader=function(pr,s){const a=attached.get(pr)||[];a.push(sources.get(s));attached.set(pr,a);return attach.call(this,pr,s);};
   const link=p.linkProgram;p.linkProgram=function(pr){const a=attached.get(pr)||[],mesh=a.some(s=>s.includes('uniform int lightCount;'));if(mesh){const kind=a.some(s=>s.includes('#define ENGINE_GL_SIMPLE_MATERIAL 1'))?'simple':'general';kinds.set(pr,kind);programProbe[kind+'Links']++;}return link.call(this,pr);};
   const use=p.useProgram;p.useProgram=function(pr){current=pr;return use.call(this,pr);};
   const loc=p.getUniformLocation;p.getUniformLocation=function(pr,name){const l=loc.call(this,pr,name);if(l)uniforms.set(l,{pr,name});return l;};
   const uniform=p.uniform1i;p.uniform1i=function(l,v){const n=uniforms.get(l);if(n?.name==='water')water.set(n.pr,v);return uniform.call(this,l,v);};
   for(const method of ['drawElements','drawArrays']){const draw=p[method];p[method]=function(...a){const kind=kinds.get(current);if(kind){programProbe[kind+'Draws']++;if(water.get(current))programProbe.waterDraws++;}return draw.apply(this,a);};}
  });
  await page.goto(urls[variant]+'/?renderer='+backend);await page.waitForFunction(()=>window.racingReady||window.racingCarError,null,{timeout:180000});assert(await page.evaluate(()=>!!window.racingReady),await page.evaluate(()=>window.racingCarError));
  assert.equal(await page.evaluate(()=>racingBackend),backend);assert.equal(await page.evaluate(()=>racingCityDetails.length),164);
  await page.evaluate(()=>{window.perfCounters=[];const original=enginePerformance;window.enginePerformance=s=>{original(s);perfCounters.push(s);};const report=EngineQuality.report;EngineQuality.report=s=>{report(s);window.lastQuality=s;};});
  await page.locator('#hover').click();await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);await page.waitForTimeout(2000);await page.evaluate(()=>Module._race_action(5));await page.waitForFunction(()=>racingState.paused);await page.waitForTimeout(3500);
  const fixed=await page.evaluate(()=>new Promise(resolve=>{const frames=[];const start=performance.now();let last=start;function tick(now){frames.push(now-last);last=now;if(now-start<8500)requestAnimationFrame(tick);else resolve({frames,snapshot:perfCounters.at(-1)});}requestAnimationFrame(tick);}));
  const id=backend+'-'+variant,entry={fixed:{...summary(fixed.frames),snapshot:fixed.snapshot},views:{},errors};report.runs[id]=entry;
  entry.identity=await page.evaluate(async()=>({build:await(await fetch('build-info.json')).json(),canvas:[Module.canvas.width,Module.canvas.height],placements:racingCityDetails,assets:racingCityKitAssets}));
  entry.wasmSha256=crypto.createHash('sha256').update(await(await page.request.get(urls[variant]+'/racing.wasm')).body()).digest('hex');
  console.log(id,'fixed',entry.fixed.fps.toFixed(1),'FPS');fs.writeFileSync(out+'/results.json',JSON.stringify(report,null,2));
  entry.probe=await page.evaluate(()=>programProbe);
  await page.screenshot({path:out+'/'+id+'-fixed.png'});
  if(backend==='webgl2'&&variant==='after'){
   assert(entry.probe.simpleDraws>0);assert(entry.probe.waterDraws>0,'Water path must still draw');
   const before=await page.evaluate(()=>programProbe.simpleLinks);
   await page.evaluate(()=>{const gl=Module.canvas.getContext('webgl2'),ext=gl.getExtension('WEBGL_lose_context');ext.loseContext();setTimeout(()=>ext.restoreContext(),250);});
   await page.waitForFunction(n=>programProbe.simpleLinks>n&&window.lastQuality?.state==='ready',before,{timeout:60000});await page.waitForTimeout(1500);
   entry.contextRecovery=await page.evaluate(()=>({probe:programProbe,quality:lastQuality,error:Module.canvas.getContext('webgl2').getError(),placements:racingCityDetails.length}));assert.equal(entry.contextRecovery.error,0);
  }
  // Drive one complete physical lap with the normal camera after the fixed views.
  await page.evaluate(()=>{racingCityReview=racingStreetReview=racingPlazaReview=-1;Module._race_action(5);window.lapFrames=[];window.recordLap=true;let last=performance.now();function tick(now){if(recordLap){lapFrames.push(now-last);last=now;requestAnimationFrame(tick);}}requestAnimationFrame(tick);
   window.pilot=setInterval(()=>{const s=racingState;if(s.phase!==2||s.paused)return;let best=Infinity,index=0;for(let i=0;i<racingTrack.length;i++){const p=racingTrack[i],d=(p[0]-s.x)**2+(p[1]-s.z)**2;if(d<best){best=d;index=i;}}const t=racingTrack[(index+6)%racingTrack.length];let a=Math.atan2(t[0]-s.x,t[1]-s.z)-s.heading;a=Math.atan2(Math.sin(a),Math.cos(a));Module._race_input(101,900,Math.max(-1,Math.min(1,-a*2)));Module._race_input(100,901,s.speed<65?1:.1);Module._race_input(102,902,s.speed>76?.5:0);},16);
  });
  await page.waitForFunction(()=>racingState.lap>=1,null,{timeout:240000});
  const lap=await page.evaluate(()=>{recordLap=false;clearInterval(pilot);Module._race_cancel();return {frames:lapFrames,state:racingState,counters:perfCounters.at(-1)?.current};});assert.equal(lap.state.recoveries,0);assert(lap.state.valid&&lap.state.best>0);entry.lap={...summary(lap.frames),state:lap.state,counters:lap.counters};
  await page.screenshot({path:out+'/'+id+'-lap.png'});assert.deepEqual(errors,[]);console.log(id,'lap',entry.lap.fps.toFixed(1),'FPS, zero recoveries/errors');fs.writeFileSync(out+'/results.json',JSON.stringify(report,null,2));await context.close();
 }
 for(const backend of ['webgl2','webgpu']){const a=report.runs[backend+'-before'],b=report.runs[backend+'-after'];assert.deepEqual(b.identity.placements,a.identity.placements);assert.deepEqual(b.identity.assets,a.identity.assets);assert.deepEqual(b.identity.canvas,a.identity.canvas);}
 }catch(e){report.failure=String(e.stack||e);throw e;}finally{fs.writeFileSync(out+'/results.json',JSON.stringify(report,null,2));await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
