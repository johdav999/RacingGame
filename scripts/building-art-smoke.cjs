// Run serially against the generated pinned-engine site.
const assert=require('node:assert/strict');
const fs=require('node:fs/promises');
const path=require('node:path');
const crypto=require('node:crypto');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8094';
const out=path.resolve(process.env.EVIDENCE_DIR||'build/evidence/building-expansion');
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const wasmHash=crypto.createHash('sha256').update(await fs.readFile(path.join(process.env.SITE_DIR||'build/web/site','racing.wasm'))).digest('hex');
 const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 const report={browser:await browser.version(),wasmHash,backends:{}};
 try{
  for(const backend of ['webgpu','webgl2']){
   const page=await browser.newPage({viewport:{width:1440,height:900}}),errors=[];
   page.on('pageerror',e=>errors.push(String(e)));
   page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
   await page.route('**/kairox-loading-warehouse.glb',route=>route.fulfill({status:200,body:Buffer.from('invalid building')}),{times:1});
   await page.goto(`${base}/?renderer=${backend}&buildingReview=-1`);
   await page.waitForFunction(()=>window.racingCarError,null,{timeout:90000});
   assert.equal(await page.evaluate(()=>!!window.racingReady),false);
   assert.equal(await page.evaluate(()=>Object.keys(racingBuildingAssets).length),6);
   await page.evaluate(()=>Module._race_action(1));await page.waitForTimeout(150);
   assert.equal(await page.evaluate(()=>racingState.phase),0,'C++ guards race entry while city is incomplete');
   await page.getByRole('button',{name:'RETRY CITY DOWNLOAD',exact:true}).click();
   await page.waitForFunction(()=>window.racingReady,null,{timeout:180000});
   assert.equal(await page.evaluate(()=>racingBackend),backend);
   const buildings=await page.evaluate(()=>({assets:racingBuildingAssets,placements:racingBuildings}));
   assert.equal(buildings.placements.length,12);
   assert.equal(Object.keys(buildings.assets).length,7);
   const counts=Array(7).fill(0);for(const p of buildings.placements)counts[p.model]++;
   assert.deepEqual(counts,[3,3,2,1,1,1,1]);
   for(const asset of Object.values(buildings.assets)){
    const record=JSON.parse(await fs.readFile(`assets/runtime/models/buildings/${asset.name}/${asset.name}.asset.json`,'utf8'));
    assert.equal(asset.sha256,record.sha256);
   }
   await page.screenshot({path:path.join(out,`${backend}-garage.png`)});
   await page.locator('#hover').click();await page.locator('#start').click();
   await page.waitForFunction(()=>racingState.phase===2);
   await page.evaluate(()=>{
    window.buildingPilot=setInterval(()=>{
     const s=racingState;if(s.phase!==2||s.paused)return;
     let best=Infinity,index=0;
     for(let i=0;i<racingTrack.length;i++){const p=racingTrack[i],d=(p[0]-s.x)**2+(p[1]-s.z)**2;if(d<best){best=d;index=i;}}
     const target=racingTrack[(index+6)%racingTrack.length];
     let angle=Math.atan2(target[0]-s.x,target[1]-s.z)-s.heading;
     angle=Math.atan2(Math.sin(angle),Math.cos(angle));
     Module._race_input(101,900,Math.max(-1,Math.min(1,-angle*2)));
     Module._race_input(100,901,s.speed<65?1:.1);
     Module._race_input(102,902,s.speed>76?.5:0);
    },16);
   });
   for(const gate of [2,4,6,8,10,0]){
    await page.waitForFunction(g=>racingState.gate===g,gate,{timeout:60000});
    await page.screenshot({path:path.join(out,`${backend}-gate-${gate}.png`)});
    console.log(`${backend} captured gate ${gate}`);
   }
   await page.waitForFunction(()=>racingState.lap>=1,null,{timeout:60000});
   await page.evaluate(()=>{clearInterval(buildingPilot);Module._race_cancel();});
   const lap=await page.evaluate(()=>racingState);
   assert.equal(lap.recoveries,0);assert(lap.valid&&lap.best>0);
   await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
   const elapsed=await page.evaluate(()=>racingState.elapsed);await page.waitForTimeout(150);
   assert.equal(await page.evaluate(()=>racingState.elapsed),elapsed);
   await page.locator('#pause-panel .garage-button').click();await page.waitForFunction(()=>racingState.phase===0);
   assert.equal(await page.evaluate(()=>racingBuildings.length),12);
   const reviewStyle=await page.addStyleTag({content:'#garage { visibility: hidden !important; }'});
   for(const model of [3,4,5,6]) {
    await page.evaluate(id=>{window.racingBuildingReview=id;},model);
    await page.waitForTimeout(300);
    await page.screenshot({path:path.join(out,`${backend}-${buildings.assets[model].name}.png`)});
   }
   await reviewStyle.evaluate(node=>node.remove());
   await page.evaluate(()=>{window.racingBuildingReview=-1;});
   assert.deepEqual(errors,[]);
   const build=await page.evaluate(async()=>await(await fetch('build-info.json')).json());
   assert.equal(build.developmentEngine,false);assert.equal(build.engineDirty,false);
   report.backends[backend]={build,buildings,lapTime:lap.best,recoveries:lap.recoveries,errors,
    checks:['last-building hash rejection and six retained imports','retry','race-entry guard','all seven building assets','12 nonduplicated placements','four landmark close-ups','complete physical lap','pause and return to garage']};
   await fs.writeFile(path.join(out,'results.json'),JSON.stringify(report,null,2));
   await page.close();console.log(backend+' buildings passed');
  }
 }catch(error){report.failure=String(error.stack||error);throw error;}
 finally{await fs.writeFile(path.join(out,'results.json'),JSON.stringify(report,null,2));await browser.close();}
})().catch(error=>{console.error(error);process.exitCode=1;});
