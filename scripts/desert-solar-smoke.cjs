// Actual generated game: both renderer backends, assets, controls and track switch.
const assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8112';
const out=path.resolve('build/evidence/desert-solar');
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files/Google/Chrome/Application/chrome.exe',args:['--force-high-performance-gpu']});
 const report={browser:await browser.version(),backends:[]};
 try {for(const backend of ['webgpu','webgl2']) {
  const page=await browser.newPage({viewport:{width:1440,height:900}}),errors=[],requests=[];
  page.setDefaultTimeout(180000);page.on('pageerror',e=>errors.push(String(e)));
  page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
  page.on('request',r=>{if(r.url().endsWith('.glb'))requests.push(r.url());});
  const ready=async()=>{await page.waitForFunction(()=>window.racingReady||window.racingCarError);assert(!await page.evaluate(()=>window.racingCarError));};
  await page.goto(`${base}/?renderer=${backend}&track=desert`);await ready();
  assert.equal(await page.evaluate(()=>racingBackend),backend);
  const result={backend,errors,assets:await page.evaluate(()=>racingDesertAssets),build:await page.evaluate(async()=>await(await fetch('build-info.json')).json())};
  assert.equal(Object.keys(result.assets).length,20);assert(!result.build.engineDirty&&!result.build.developmentEngine);
  assert(!requests.some(url=>/\/(buildings|city-kit|street-kit|track-kit|plaza)\//.test(url)));
  result.wasmSha256=await page.evaluate(async()=>Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',await(await fetch('racing.wasm')).arrayBuffer())),b=>b.toString(16).padStart(2,'0')).join(''));
  await page.waitForTimeout(3000);await page.screenshot({path:path.join(out,`${backend}-garage.png`)});
  result.garageFps=await page.locator('#fps-counter').textContent();
  for(const name of ['car','hover','bastion','vector']) {
   await page.locator('#'+name).click();await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);
   const start=await page.evaluate(()=>racingState);
   await page.keyboard.down('w');await page.waitForFunction(()=>racingState.speed>35);await page.keyboard.up('w');
   assert(await page.evaluate(s=>Math.hypot(racingState.x-s.x,racingState.z-s.z)>3,start));
   await page.waitForFunction(s=>racingState.standings.filter(r=>r.id).every(r=>{const p=s.standings.find(p=>p.id===r.id);return Math.hypot(r.x-p.x,r.z-p.z)>1;}),start,{timeout:10000});
   await page.keyboard.down('f');await page.waitForFunction(()=>racingState.combat.shots>0);await page.keyboard.up('f');
   await page.keyboard.press('r');await page.waitForFunction(()=>racingState.recoveries===1);
   await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
   const paused=await page.evaluate(()=>racingState.elapsed);await page.waitForTimeout(300);
   assert.equal(await page.evaluate(()=>racingState.elapsed),paused);
   await page.screenshot({path:path.join(out,`${backend}-${name}-race.png`),style:'#pause-panel{visibility:hidden!important}'});
   await page.locator('#pause-panel .garage-button').click();await page.waitForFunction(()=>racingState.phase===0);
   console.log(backend,name,'controls passed');
  }
  await page.setViewportSize({width:390,height:844});await page.screenshot({path:path.join(out,`${backend}-narrow.png`)});
  assert(await page.locator('#track-select').isVisible());assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
  await page.setViewportSize({width:1440,height:900});
  const career=await page.evaluate(()=>localStorage.getItem('neon-ascension.career.v1'));
  await page.locator('#track-select').selectOption('kairox');await page.waitForURL('**track=kairox');await ready();
  assert.equal(await page.evaluate(()=>racingTrackKind),'kairox');assert.equal(await page.evaluate(()=>racingBackend),backend);
  assert.equal(await page.evaluate(()=>localStorage.getItem('neon-ascension.career.v1')),career);
  assert.equal(Object.keys(await page.evaluate(()=>racingDesertAssets)).length,0);
  await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);
  await page.screenshot({path:path.join(out,`${backend}-kairox-regression.png`)});
  await page.goto(`${base}/?renderer=${backend}&track=desert&desertReview=10`);await ready();
  for(const sample of [10,36,43,70,100,150]) {
   await page.evaluate(i=>{window.racingDesertReview=i;},sample);await page.waitForTimeout(700);
   await page.screenshot({path:path.join(out,`${backend}-sector-${sample}.png`),style:'header,main,footer,.vignette{visibility:hidden!important}'});
  }
  assert.deepEqual(errors,[]);report.backends.push(result);
  await fs.writeFile(path.join(out,'report.json'),JSON.stringify(report,null,2));await page.close();
 }}catch(e){report.failure=String(e.stack||e);throw e;}
 finally{await fs.writeFile(path.join(out,'report.json'),JSON.stringify(report,null,2));await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
