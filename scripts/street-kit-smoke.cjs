// Verify the generated pinned-engine site, one renderer at a time.
const assert=require('node:assert/strict');
const fs=require('node:fs/promises');
const path=require('node:path');
const crypto=require('node:crypto');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8189';
const site=process.env.SITE_DIR||'build/web/site';
const out=path.resolve('build/evidence/street-kit');
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:process.env.BROWSER_EXECUTABLE||'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 const report={browser:await browser.version(),site,base,wasmHash:crypto.createHash('sha256').update(await fs.readFile(path.join(site,'racing.wasm'))).digest('hex'),backends:{}};
 let activePage;
 try{
  for(const backend of ['webgpu','webgl2']){
   const page=await browser.newPage({viewport:{width:1440,height:900}}),errors=[];
   activePage=page;
   page.on('pageerror',error=>{errors.push(String(error));console.log('Page error:',String(error));});
   page.on('console',message=>{if(message.type()==='error'){errors.push(message.text());console.log('Console error:',message.text());}});
   await page.route('**/kairox-utility-cabinet.glb',route=>route.fulfill({status:200,body:Buffer.from('invalid street asset')}),{times:1});
   await page.goto(`${base}/?renderer=${backend}`);
   console.log(backend+' loaded page');
   await page.waitForFunction(()=>window.racingCarError,null,{timeout:120000});
   assert.equal(await page.evaluate(()=>!!window.racingReady),false);
   assert.equal(await page.evaluate(()=>Object.keys(racingStreetAssets).length),11);
   await page.evaluate(()=>Module._race_action(1));await page.waitForTimeout(150);
   assert.equal(await page.evaluate(()=>racingState.phase),0);
   await page.getByRole('button',{name:'RETRY CITY DOWNLOAD',exact:true}).click();
   await page.waitForFunction(()=>window.racingReady||window.racingCarError,null,{timeout:120000});
   assert(await page.evaluate(()=>!!window.racingReady),await page.evaluate(()=>window.racingCarError||'Scene did not become ready'));
   console.log(backend+' loaded all scene assets');
   assert.equal(await page.evaluate(()=>racingBackend),backend);
   const assets=await page.evaluate(()=>racingStreetAssets),placements=await page.evaluate(()=>racingStreetPlacements);
   assert.equal(Object.keys(assets).length,12);
   for(const asset of Object.values(assets)){
    const record=JSON.parse(await fs.readFile(`assets/runtime/models/street-kit/${asset.name}/${asset.name}.asset.json`,'utf8'));
    assert.equal(asset.sha256,record.sha256);
   }
   for(let id=0;id<12;id++)assert(placements.some(p=>p.model===id));
   await page.screenshot({path:path.join(out,`${backend}-market.png`)});
   await page.locator('#hover').click();await page.locator('#start').click();
   await page.waitForFunction(()=>racingState.phase===2);
   await page.evaluate(()=>{
    window.streetSample=0;
    window.streetPilot=setInterval(()=>{
     const s=racingState;if(s.phase!==2||s.paused)return;
     let best=Infinity,index=0;
     for(let i=0;i<racingTrack.length;i++){const p=racingTrack[i],d=(p[0]-s.x)**2+(p[1]-s.z)**2;if(d<best){best=d;index=i;}}
     window.streetSample=index;
     const target=racingTrack[(index+6)%racingTrack.length];
     let angle=Math.atan2(target[0]-s.x,target[1]-s.z)-s.heading;angle=Math.atan2(Math.sin(angle),Math.cos(angle));
     Module._race_input(101,900,Math.max(-1,Math.min(1,-angle*2)));
     Module._race_input(100,901,s.speed<65?1:.1);Module._race_input(102,902,s.speed>76?.5:0);
    },16);
   });
   for(const sample of [24,49,54,60,90,120,164]){
    await page.waitForFunction(i=>streetSample>=i&&streetSample<i+8,sample,{timeout:60000});
    await page.screenshot({path:path.join(out,`${backend}-sample-${sample}.png`)});
   }
   await page.waitForFunction(()=>racingState.lap>=1,null,{timeout:60000});
   await page.evaluate(()=>{clearInterval(streetPilot);Module._race_cancel();});
   const lap=await page.evaluate(()=>racingState);
   assert.equal(lap.recoveries,0);assert(lap.valid&&lap.best>0);
   assert.deepEqual(errors,[]);
   const build=await page.evaluate(async()=>await(await fetch('build-info.json')).json());
   assert.equal(build.developmentEngine,false);assert.equal(build.engineDirty,false);
   report.backends[backend]={build,assets,placements,lapTime:lap.best,errors,checks:['12 hashes and imports','failed load retains partial scene','retry and race-entry guard','all modules used','underpass traversal','physical lap without recovery']};
   await fs.writeFile(path.join(out,'results.json'),JSON.stringify(report,null,2));
   await page.close();console.log(backend+' street kit passed');
  }
  assert.equal(crypto.createHash('sha256').update(await fs.readFile(path.join(site,'racing.wasm'))).digest('hex'),report.wasmHash,'Generated build changed during renderer validation');
 }catch(error){report.failure=String(error.stack||error);
  if(activePage&&!activePage.isClosed()){
   report.failureState=await activePage.evaluate(()=>({status:document.getElementById('status')?.textContent,error:window.racingCarError,ready:window.racingReady,street:window.racingStreetAssets,track:window.racingTrackAssets})).catch(()=>null);
   await activePage.screenshot({path:path.join(out,'failure.png')}).catch(()=>{});
  }
  throw error;}
 finally{await fs.writeFile(path.join(out,'results.json'),JSON.stringify(report,null,2));await browser.close();}
})().catch(error=>{console.error(error);process.exitCode=1;});
