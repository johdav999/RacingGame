const assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path'),crypto=require('node:crypto');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8139',out=path.resolve('build/evidence/city-ground');
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const manifest=JSON.parse(await fs.readFile('build/city-ground-site/validation-manifest.json','utf8'));
 const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 const report={wasmSha256:manifest.wasmSha256,backends:{},errors:[]};
 try{for(const backend of ['webgpu','webgl2']){
  const context=await browser.newContext({viewport:{width:1440,height:900}}),page=await context.newPage();
  page.on('pageerror',e=>report.errors.push(String(e)));page.on('console',m=>{if(m.type()==='error')report.errors.push(m.text());});
  const ready=()=>page.waitForFunction(()=>window.racingReady&&window.racingState,null,{timeout:180000});
  const shot=name=>page.screenshot({path:path.join(out,`${backend}-${name}.png`)});
  await page.goto(`${base}/?renderer=${backend}&groundReview=0`);await ready();
  assert.equal(await page.evaluate(()=>racingBackend),backend);
  const hash=crypto.createHash('sha256').update(await(await page.request.get(`${base}/racing.wasm`)).body()).digest('hex');assert.equal(hash,report.wasmSha256);
  const stats=await page.evaluate(()=>racingCityGround);
  assert.equal(stats.walks,332);assert.equal(stats.drains,332);assert.equal(stats.clusters,12);assert.equal(stats.docks,4);assert.equal(stats.vans,3);
  assert(stats.shadows>=170&&stats.damp>=12&&stats.pools===40);assert(stats.triangles<60000&&stats.meshes<160);
  await page.addStyleTag({content:'#garage {visibility:hidden!important}'});
  for(const [id,name] of [[0,'transit-ground'],[1,'loading-dock'],[2,'service-detail'],[3,'trackside'],[4,'service-vehicle']]) {
   await page.evaluate(id=>racingGroundReview=id,id);await page.waitForTimeout(800);await shot(name);
  }
  // Use the existing transit authoring camera for a matching before/after view.
  await page.goto(`${base}/?renderer=${backend}&streetReview=0`);await ready();
  await page.addStyleTag({content:'#garage,.street-tour {visibility:hidden!important}'});await page.waitForTimeout(800);await shot('underpass');
  await page.goto(`${base}/?renderer=${backend}`);await ready();
  await page.locator('#hover').click();await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);
  await page.keyboard.down('w');await page.waitForFunction(()=>racingState.speed>10,null,{timeout:15000});await page.keyboard.up('w');
  await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);await shot('driving');
  await page.keyboard.press('Escape');await page.waitForFunction(()=>!racingState.paused);
  await page.evaluate(()=>{
   window.groundPilot=setInterval(()=>{
    const s=racingState;if(s.phase!==2||s.paused)return;let best=Infinity,index=0;
    for(let i=0;i<racingTrack.length;i++){const p=racingTrack[i],d=(p[0]-s.x)**2+(p[1]-s.z)**2;if(d<best){best=d;index=i;}}
    const t=racingTrack[(index+6)%racingTrack.length];let a=Math.atan2(t[0]-s.x,t[1]-s.z)-s.heading;a=Math.atan2(Math.sin(a),Math.cos(a));
    Module._race_input(101,900,Math.max(-1,Math.min(1,-a*2)));Module._race_input(100,901,s.speed<65?1:.1);Module._race_input(102,902,s.speed>76?.5:0);
   },16);
  });
  for(const gate of [3,7]) {await page.waitForFunction(g=>racingState.gate===g,gate,{timeout:90000});await shot(`race-gate-${gate}`);}
  await page.waitForFunction(()=>racingState.lap>=1,null,{timeout:120000});
  await page.evaluate(()=>{clearInterval(groundPilot);Module._race_cancel();});
  const state=await page.evaluate(()=>racingState);assert.equal(state.recoveries,0);assert(state.valid&&state.best>0);
  report.backends[backend]={stats,lapTime:state.best,recoveries:state.recoveries,actualBackend:await page.evaluate(()=>racingBackend)};
  assert.deepEqual(report.errors,[]);await context.close();console.log(backend+' city ground passed');
  await fs.writeFile(path.join(out,'results.json'),JSON.stringify(report,null,2));
 }}catch(e){report.failure=String(e.stack||e);throw e;}
 finally{await fs.writeFile(path.join(out,'results.json'),JSON.stringify(report,null,2));await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
