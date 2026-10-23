// Run serially on the generated site. Uses real input, physics and rendered wheel matrices.
const assert=require('node:assert/strict');
const fs=require('node:fs/promises');
const path=require('node:path');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8088';
const out=path.resolve(process.env.EVIDENCE_DIR||'build/evidence/cybercar');
const dot=(a,b)=>a.reduce((sum,x,i)=>sum+x*b[i],0);
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:process.env.BROWSER_EXECUTABLE||'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 const report={browser:await browser.version(),backends:{}};
 try {
  for(const backend of ['webgpu','webgl2']) {
   const page=await browser.newPage({viewport:{width:1440,height:900}});
   const errors=[];page.on('pageerror',e=>errors.push(String(e)));
   page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
   // A bad download must not silently launch the placeholder car.
   await page.route('**/cybercar-clean-detail/cybercar.glb',route=>route.fulfill({
    status:200,contentType:'model/gltf-binary',body:Buffer.from('invalid asset')
   }),{times:1});
   await page.goto(`${base}/?renderer=${backend}&track=${process.env.TRACK||'kairox'}`);
   await page.waitForFunction(()=>window.racingCarError);
   assert.equal(await page.evaluate(()=>!!window.racingReady),false);
   await page.getByRole('button',{name:'RETRY CAR DOWNLOAD'}).click();
   await page.waitForFunction(()=>window.racingReady&&window.racingCarAsset?.loaded,null,{timeout:90000});
   assert.equal(await page.evaluate(()=>racingBackend),backend);
   const asset=await page.evaluate(()=>racingCarAsset);
   const expected=JSON.parse(await fs.readFile('assets/runtime/models/cybercar-clean-detail/cybercar.asset.json','utf8'));
   assert.equal(asset.sha256,expected.sha256);
   const names=['wheel_front_left','wheel_front_right','wheel_rear_left','wheel_rear_right'];
   assert.deepEqual(await page.evaluate(()=>racingWheels.map(w=>w.name)),names);
   await page.screenshot({path:path.join(out,`${backend}-garage.png`)});
   await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);
   await page.keyboard.down('w');await page.waitForFunction(()=>racingState.speed>20);
   const before=await page.evaluate(()=>structuredClone(racingWheels));
   const samples=[];
   // Multiple frames avoid mistaking a complete revolution for a stationary mesh.
   for(let frame=0;frame<5;frame++) {
    await page.waitForTimeout(70);
    samples.push(await page.evaluate(()=>structuredClone(racingWheels)));
   }
   const rolling=samples.at(-1);
   for(let i=0;i<4;i++){
    assert(samples.some(s=>Math.abs(s[i].spin-before[i].spin)>.02),'Every wheel spins while driving');
    assert(Math.abs(rolling[i].omega)>1);
    assert(samples.some(s=>dot(s[i].up,before[i].up)<.999),'The rendered wheel orientation changes');
   }
   await page.keyboard.down('d');await page.waitForTimeout(400);
   const steered=await page.evaluate(()=>structuredClone(racingWheels));
   for(let i=0;i<2;i++){
    assert(Math.abs(steered[i].steer)>.03,'Both front wheels steer');
    assert(dot(steered[i].axle,steered[i+2].axle)<.999,'Front mesh axles turn relative to rear mesh axles');
   }
   assert(Math.abs(steered[2].steer)<1e-6);assert(Math.abs(steered[3].steer)<1e-6);
   await page.screenshot({path:path.join(out,`${backend}-steering.png`)});
   await page.keyboard.up('d');await page.keyboard.up('w');
   await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
   await page.waitForTimeout(150);const paused=await page.evaluate(()=>structuredClone(racingWheels));
   await page.waitForTimeout(250);assert.deepEqual(await page.evaluate(()=>racingWheels),paused,'Pause freezes wheel presentation');
   await page.locator('#resume').click();await page.waitForFunction(()=>!racingState.paused);
   await page.keyboard.press('r');await page.waitForFunction(()=>racingState.recoveries===1);
   assert((await page.evaluate(()=>racingWheels)).every(w=>Number.isFinite(w.spin)&&w.center.every(Number.isFinite)));
   await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
   await page.locator('#pause-panel .garage-button').click();await page.waitForFunction(()=>racingState.phase===0);
   await page.locator('#hover').click();await page.waitForFunction(()=>racingState.kind===1);
   await page.locator('#car').click();await page.waitForFunction(()=>racingState.kind===0);
   assert.deepEqual(await page.evaluate(()=>racingWheels.map(w=>w.name)),names);
   await page.setViewportSize({width:390,height:844});
   await page.screenshot({path:path.join(out,`${backend}-narrow.png`)});
   assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),true);
   assert.deepEqual(errors,[]);
   report.backends[backend]={asset,build:await page.evaluate(async()=>await(await fetch('build-info.json')).json()),
    rolling,steered,errors,checks:['invalid download and retry','exact GLB hash','four named mesh bindings','physics spin and rendered rotation',
     'front-only steering and rendered axles','pause','recovery','hover/car selection','narrow viewport']};
   await page.close();console.log(`${backend}: cybercar passed`);
  }
 }catch(e){report.failure=String(e.stack||e);throw e;}
 finally{await fs.writeFile(path.join(out,'results.json'),JSON.stringify(report,null,2));await browser.close();}
})();
