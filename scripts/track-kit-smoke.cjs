// Run serially against a frozen generated site. No generation/provider calls.
const assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8098';
const out=path.resolve(process.env.EVIDENCE_DIR||'build/evidence/track-kit');
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:process.env.BROWSER_EXECUTABLE||'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 const report={browser:await browser.version(),backends:{}};
 try {for(const backend of ['webgpu','webgl2']) {
  const context=await browser.newContext({viewport:{width:1440,height:900}}),page=await context.newPage(),errors=[];
  page.on('pageerror',e=>errors.push(String(e)));page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
  await page.route('**/kairox-storm-drain.glb',route=>route.fulfill({status:200,contentType:'model/gltf-binary',body:'invalid asset'}),{times:1});
  await page.goto(`${base}/?renderer=${backend}&trackReview=0`);
  await page.waitForFunction(()=>window.racingCarError?.includes('out of date'),null,{timeout:180000});
  assert.equal(await page.evaluate(()=>!!window.racingReady),false);
  assert.equal(await page.evaluate(()=>Object.keys(racingTrackAssets).length),12);
  await page.evaluate(()=>Module._race_action(1));await page.waitForTimeout(100);
  assert.equal(await page.evaluate(()=>racingState.phase),0);
  await page.getByRole('button',{name:'RETRY TRACK DOWNLOAD',exact:true}).click();
  await page.waitForFunction(()=>window.racingReady,null,{timeout:120000});
  assert.equal(await page.evaluate(()=>racingBackend),backend);
  const assets=await page.evaluate(()=>racingTrackAssets);
  assert.equal(Object.keys(assets).length,13);
  await page.addStyleTag({content:'#garage {visibility:hidden!important}'});
  for(const [id,asset] of Object.entries(assets)) {
   const record=JSON.parse(await fs.readFile(`assets/runtime/models/track-kit/${asset.name}/${asset.name}.asset.json`,'utf8'));
   assert.equal(asset.sha256,record.sha256);
   await page.evaluate(id=>window.racingTrackReview=Number(id),id);await page.waitForTimeout(200);
   await page.screenshot({path:path.join(out,`${backend}-${asset.name}.jpg`)});
  }
  const placements=await page.evaluate(()=>racingTrackPlacements),counts=Array(13).fill(0);
  for(const p of placements)counts[p.model]++;
  assert(counts.every(n=>n>0));assert.equal(counts[0],192);assert.equal(counts[3],1);assert.equal(counts[4],11);
  await page.goto(`${base}/?renderer=${backend}`);
  await page.waitForFunction(()=>window.racingReady,null,{timeout:180000});
  await page.screenshot({path:path.join(out,`${backend}-garage.jpg`)});
  await page.locator('#hover').click();await page.locator('#start').click();
  await page.waitForFunction(()=>racingState.phase===2);
  await page.evaluate(()=>{
   window.trackPilot=setInterval(()=>{
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
   await page.waitForFunction(g=>racingState.gate===g,gate,{timeout:120000});
   await page.screenshot({path:path.join(out,`${backend}-gate-${gate}.jpg`)});
  }
  await page.waitForFunction(()=>racingState.lap>=1,null,{timeout:120000});
  await page.evaluate(()=>{clearInterval(trackPilot);Module._race_cancel();Module._race_action(5);});
  await page.waitForFunction(()=>racingState.paused);
  const state=await page.evaluate(()=>racingState);assert.equal(state.recoveries,0);assert(state.valid&&state.best>0);
  await page.waitForTimeout(200);assert.equal(await page.evaluate(()=>racingState.elapsed),state.elapsed);
  await page.screenshot({path:path.join(out,`${backend}-lap.jpg`)});
  assert.deepEqual(errors,[]);
  report.backends[backend]={assets,counts,placements:placements.length,lap:state.best,recoveries:state.recoveries,errors};
  await context.close();
 }} finally {await browser.close();}
 await fs.writeFile(path.join(out,'report.json'),JSON.stringify(report,null,2));
 console.log(JSON.stringify(report,null,2));
})().catch(e=>{console.error(e);process.exitCode=1;});
