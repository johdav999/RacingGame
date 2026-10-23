// Exact generated site, real Chromium, renderer backends tested serially.
const assert=require('node:assert/strict');
const fs=require('node:fs/promises');
const path=require('node:path');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8189';
const out=path.resolve(process.env.EVIDENCE_DIR||'build/evidence/city-kit');
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:process.env.BROWSER_EXECUTABLE||'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 const report={browser:await browser.version(),backends:{}};
 try {
  for(const backend of ['webgpu','webgl2']) {
   const context=await browser.newContext({viewport:{width:1440,height:900}});
   const page=await context.newPage(),errors=[];
   page.on('pageerror',e=>errors.push(String(e)));
   page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
   await page.route('**/kairox-crate-stack.glb',route=>route.fulfill({status:200,contentType:'model/gltf-binary',body:Buffer.from('bad model')}),{times:1});
   await page.goto(`${base}/?renderer=${backend}&cityReview=0`);
   await page.waitForFunction(()=>window.racingCarError,null,{timeout:120000});
   assert(!await page.evaluate(()=>!!window.racingReady));
   assert.equal(await page.evaluate(()=>Object.keys(racingCityKitAssets).length),3);
   await page.getByRole('button',{name:'RETRY CITY DOWNLOAD',exact:true}).click();
   await page.waitForFunction(()=>window.racingReady&&window.racingState,null,{timeout:120000});
   assert.equal(await page.evaluate(()=>racingBackend),backend);
   const assets=await page.evaluate(()=>racingCityKitAssets);
   assert.equal(Object.keys(assets).length,14);
   for(const [id,asset] of Object.entries(assets)) {
    const record=JSON.parse(await fs.readFile(`assets/runtime/models/city-kit/${asset.name}/${asset.name}.asset.json`,'utf8'));
    assert.equal(asset.sha256,record.sha256);
    await page.evaluate(id=>{window.racingCityReview=Number(id);},id);
    await page.waitForTimeout(800);
    // Hide only the garage overlay for asset inspection; keep the actual scene.
    await page.addStyleTag({content:'#garage { visibility: hidden !important; }'});
    await page.screenshot({path:path.join(out,`${backend}-${asset.name}.png`)});
   }
   const placements=await page.evaluate(()=>racingCityDetails);
   assert.equal(placements.length,164);
   const counts=Array(14).fill(0);for(const p of placements)counts[p.model]++;
   assert.deepEqual(counts.slice(0,6),[2,2,2,2,4,1]);
   assert.deepEqual(counts.slice(8,12),[1,1,1,1]);
   assert.deepEqual(counts,[2,2,2,2,4,1,26,25,1,1,1,1,48,48]);
   for(const p of placements.filter(p=>p.model>=12||((p.model===6||p.model===7)&&p.scale<1))) {
    assert(p.radius<=15.01&&p.scale>0&&p.scale<=1);
    for(const other of placements)if(p!==other)
     assert(Math.hypot(p.x-other.x,p.z-other.z)>p.radius+other.radius);
   }
   const utility=placements.find(p=>p.model===11);
   assert.deepEqual([utility.x,utility.y,utility.z],[20,0,25]);
   assert(placements.filter(p=>p.model>=12).every(p=>p.radius<=15.01&&p.scale>0&&p.scale<=1));
   await page.goto(`${base}/?renderer=${backend}`);
   await page.waitForFunction(()=>window.racingReady&&window.racingState,null,{timeout:120000});
   await page.screenshot({path:path.join(out,`${backend}-garage.png`)});
   await page.locator('#hover').click();await page.locator('#start').click();
   await page.waitForFunction(()=>racingState.phase===2);
   await page.evaluate(()=>{
    window.cityFrames=[];window.cityCounters=[];
    const original=window.enginePerformance;
    window.enginePerformance=s=>{original(s);cityCounters.push(s.current);};
    let last=0;function frame(now){if(last)cityFrames.push(now-last);last=now;requestAnimationFrame(frame);}requestAnimationFrame(frame);
   });
   await page.evaluate(()=>{
    window.cityPilot=setInterval(()=>{
     const s=racingState;if(s.phase!==2||s.paused)return;
     let best=Infinity,index=0;
     for(let i=0;i<racingTrack.length;i++){const p=racingTrack[i],d=(p[0]-s.x)**2+(p[1]-s.z)**2;if(d<best){best=d;index=i;}}
     const t=racingTrack[(index+6)%racingTrack.length];let a=Math.atan2(t[0]-s.x,t[1]-s.z)-s.heading;
     a=Math.atan2(Math.sin(a),Math.cos(a));
     Module._race_input(101,900,Math.max(-1,Math.min(1,-a*2)));
     Module._race_input(100,901,s.speed<65?1:.1);Module._race_input(102,902,s.speed>76?.5:0);
    },16);
   });
   for(const gate of [2,4,6,8,10,0]) {
    await page.waitForFunction(g=>racingState.gate===g,gate,{timeout:180000});
    await page.screenshot({path:path.join(out,`${backend}-gate-${gate}.png`)});
   }
   await page.waitForFunction(()=>racingState.lap>=1,null,{timeout:90000});
   await page.evaluate(()=>{clearInterval(cityPilot);Module._race_cancel();});
   const state=await page.evaluate(()=>racingState);assert.equal(state.recoveries,0);assert(state.valid&&state.best>0);
   const build=await page.evaluate(async()=>await(await fetch('build-info.json')).json());
   assert.equal(build.developmentEngine,false);assert.equal(build.engineDirty,false);assert.deepEqual(errors,[]);
   const performance=await page.evaluate(()=>({fps:1000/(cityFrames.reduce((s,x)=>s+x,0)/cityFrames.length),frames:cityFrames.length,lodParts:Math.max(...cityCounters.map(c=>c.cameraLodParts)),maxPasses:Math.max(...cityCounters.map(c=>c.passes))}));
   assert(performance.lodParts>0&&performance.maxPasses<=3);
   report.backends[backend]={assets,placements,build,lapTime:state.best,recoveries:state.recoveries,performance,errors};
   await fs.writeFile(path.join(out,'results.json'),JSON.stringify(report,null,2));
   await context.close();console.log(backend+' city kit passed');
  }
 }catch(e){report.failure=String(e.stack||e);throw e;}
 finally{await fs.writeFile(path.join(out,'results.json'),JSON.stringify(report,null,2));await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
