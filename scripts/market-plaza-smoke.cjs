const assert=require('node:assert/strict');
const fs=require('node:fs/promises');
const path=require('node:path');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8097';
const out=path.resolve('build/evidence/market-plaza');
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 const report={backends:{}};
 try{for(const backend of ['webgpu','webgl2']){
  const context=await browser.newContext({viewport:{width:1440,height:900}});
  const page=await context.newPage(),errors=[];
  page.on('pageerror',e=>errors.push(String(e)));
  page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
  await page.route('**/kairox-plaza-drain.glb',r=>r.fulfill({status:200,body:Buffer.from('bad')}),{times:1});
  await page.goto(`${base}/?renderer=${backend}&plazaReview=0`);
  await page.waitForFunction(()=>window.racingCarError,null,{timeout:180000});
  assert(!await page.evaluate(()=>!!window.racingReady));
  assert.equal(await page.evaluate(()=>Object.keys(racingPlazaAssets).length),1);
  await page.getByRole('button',{name:'RETRY CITY DOWNLOAD',exact:true}).click();
  await page.waitForFunction(()=>window.racingReady&&window.racingState,null,{timeout:180000});
  assert.equal(await page.evaluate(()=>racingBackend),backend);
  const assets=await page.evaluate(()=>racingPlazaAssets);
  assert.equal(Object.keys(assets).length,2);
  for(const asset of Object.values(assets)) {
   const record=JSON.parse(await fs.readFile(`assets/runtime/models/plaza/${asset.name}/${asset.name}.asset.json`,'utf8'));
   assert.equal(asset.sha256,record.sha256);
  }
  await page.addStyleTag({content:'#garage {visibility:hidden!important}'});
  for(let i=0;i<4;i++){
   await page.evaluate(i=>window.racingPlazaReview=i,i);await page.waitForTimeout(600);
   await page.screenshot({path:path.join(out,`${backend}-plaza-${i}.png`)});
  }
  const build=await page.evaluate(async()=>await(await fetch('build-info.json')).json());
  await page.goto(`${base}/?renderer=${backend}`);
  await page.waitForFunction(()=>window.racingReady&&window.racingState,null,{timeout:180000});
  await page.locator('#hover').click();await page.locator('#start').click();
  await page.waitForFunction(()=>racingState.phase===2);
  await page.evaluate(()=>{
   window.plazaPilot=setInterval(()=>{
    const s=racingState;if(s.phase!==2||s.paused)return;
    let best=Infinity,index=0;
    for(let i=0;i<racingTrack.length;i++){const p=racingTrack[i],d=(p[0]-s.x)**2+(p[1]-s.z)**2;if(d<best){best=d;index=i;}}
    const t=racingTrack[(index+6)%racingTrack.length];let a=Math.atan2(t[0]-s.x,t[1]-s.z)-s.heading;
    a=Math.atan2(Math.sin(a),Math.cos(a));
    Module._race_input(101,900,Math.max(-1,Math.min(1,-a*2)));
    Module._race_input(100,901,s.speed<65?1:.1);Module._race_input(102,902,s.speed>76?.5:0);
   },16);
  });
  for(const gate of [2,7]){
   await page.waitForFunction(g=>racingState.gate===g,gate,{timeout:90000});
   await page.screenshot({path:path.join(out,`${backend}-race-${gate}.png`)});
  }
  await page.waitForFunction(()=>racingState.lap>=1,null,{timeout:120000});
  await page.evaluate(()=>{clearInterval(plazaPilot);Module._race_cancel();});
  const state=await page.evaluate(()=>racingState);assert.equal(state.recoveries,0);assert(state.valid&&state.best>0);
  assert(!build.developmentEngine&&!build.engineDirty);assert.deepEqual(errors,[]);
  report.backends[backend]={assets,build,lapTime:state.best,recoveries:state.recoveries,errors};
  await fs.writeFile(path.join(out,'results.json'),JSON.stringify(report,null,2));
  await context.close();console.log(backend+' plaza passed');
 }}catch(e){report.failure=String(e.stack||e);throw e;}
 finally{await fs.writeFile(path.join(out,'results.json'),JSON.stringify(report,null,2));await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
