const assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path'),crypto=require('node:crypto');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8142',out=path.resolve('build/evidence/desert-circuits');
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:process.env.BROWSER_EXECUTABLE||'C:/Program Files/Google/Chrome/Application/chrome.exe',args:['--force-high-performance-gpu']});
 const report={browser:await browser.version(),wasmSha256:crypto.createHash('sha256').update(await fs.readFile('build/web/site/racing.wasm')).digest('hex'),circuits:[]};
 try {for(const backend of ['webgpu','webgl2']) {
  const page=await browser.newPage({viewport:{width:1440,height:900}}),errors=[],requests=[];
  page.setDefaultTimeout(180000);page.on('pageerror',e=>errors.push(String(e)));page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
  page.on('request',r=>{if(r.url().endsWith('.glb'))requests.push(r.url());});
  const ready=async()=>{await page.waitForFunction(()=>window.racingReady||window.racingCarError);assert(!await page.evaluate(()=>window.racingCarError));};
  let previousRoute;
  for(const [slug,label] of [['sirocco','DUNE SWEEP'],['canyon','TWIN MESA']]) {
   await page.goto(`${base}/?renderer=${backend}&track=${slug}`);await ready();
   assert.equal(await page.evaluate(()=>racingBackend),backend);assert.equal(await page.evaluate(()=>racingTrackKind),slug);
   const route=await page.evaluate(()=>racingTrack);assert.equal(route.length,192);if(previousRoute)assert.notDeepEqual(route,previousRoute);previousRoute=route;
   const assets=await page.evaluate(()=>racingDesertAssets);assert.equal(Object.keys(assets).length,20);
   assert.equal((await page.locator('.event .tag').textContent()),label);
   assert(!requests.some(url=>/\/(buildings|city-kit|street-kit|track-kit|plaza)\//.test(url)));
   const hash=await page.evaluate(async()=>Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',await(await fetch('racing.wasm')).arrayBuffer())),b=>b.toString(16).padStart(2,'0')).join(''));
   assert.equal(hash,report.wasmSha256);
   const r={backend,slug,assets:Object.values(assets).map(a=>a.name),length:route.reduce((n,p,i)=>n+Math.hypot(p[0]-route[(i+1)%192][0],p[1]-route[(i+1)%192][1]),0),build:await page.evaluate(async()=>await(await fetch('build-info.json')).json())};
   assert(!r.build.engineDirty&&!r.build.developmentEngine);
   await page.screenshot({path:path.join(out,`${backend}-${slug}-garage.png`)});
   for(const name of ['car','hover','bastion','vector']) {
    await page.locator('#'+name).click();await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);
    const start=await page.evaluate(()=>racingState);await page.keyboard.down('w');await page.waitForFunction(()=>racingState.speed>35);await page.keyboard.up('w');
    assert(await page.evaluate(s=>Math.hypot(racingState.x-s.x,racingState.z-s.z)>3,start));
    await page.waitForFunction(s=>racingState.standings.filter(r=>r.id).every(r=>{const p=s.standings.find(p=>p.id===r.id);return Math.hypot(r.x-p.x,r.z-p.z)>1;}),start,{timeout:20000});
    await page.keyboard.down('f');await page.waitForFunction(()=>racingState.combat.shots>0);await page.keyboard.up('f');
    await page.keyboard.press('r');await page.waitForFunction(()=>racingState.recoveries===1);
    await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
    const paused=await page.evaluate(()=>racingState.elapsed);await page.waitForTimeout(150);assert.equal(await page.evaluate(()=>racingState.elapsed),paused);
    if(name==='car')await page.screenshot({path:path.join(out,`${backend}-${slug}-race.png`),style:'#pause-panel{visibility:hidden!important}'});
    await page.locator('#pause-panel .garage-button').click();await page.waitForFunction(()=>racingState.phase===0);
   }
   await page.locator('#mode-select').selectOption('trial');await page.waitForFunction(()=>racingState.trial.active);
   await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);await page.keyboard.press('r');await page.waitForFunction(()=>!racingState.trial.clean);
   await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);await page.locator('#pause-panel .garage-button').click();await page.waitForFunction(()=>racingState.phase===0);
   await page.setViewportSize({width:390,height:844});assert(await page.locator('#track-select').isVisible());assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
   await page.screenshot({path:path.join(out,`${backend}-${slug}-narrow.png`)});await page.setViewportSize({width:1440,height:900});
   const career=await page.evaluate(()=>localStorage.getItem('neon-ascension.career.v1'));
   await page.locator('#track-select').selectOption(slug==='sirocco'?'canyon':'sirocco');await ready();assert.equal(await page.evaluate(()=>localStorage.getItem('neon-ascension.career.v1')),career);assert.equal(await page.evaluate(()=>racingBackend),backend);
   await page.goto(`${base}/?renderer=${backend}&track=${slug}&desertReview=10`);await ready();
   for(const sample of [10,43,75,100,150]){await page.evaluate(i=>window.racingDesertReview=i,sample);await page.waitForTimeout(600);await page.screenshot({path:path.join(out,`${backend}-${slug}-sector-${sample}.png`),style:'header,main,footer,.vignette{visibility:hidden!important}'});}
   report.circuits.push(r);console.log(backend,slug,r.length.toFixed(1),'m passed');await fs.writeFile(path.join(out,'report.json'),JSON.stringify(report,null,2));
  }
  await page.goto(`${base}/?renderer=${backend}&track=desert`);await ready();assert.equal(await page.evaluate(()=>racingTrackKind),'desert');await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);
  assert.deepEqual(errors,[]);await page.close();
 }}catch(e){report.failure=String(e.stack||e);throw e;}finally{await fs.writeFile(path.join(out,'report.json'),JSON.stringify(report,null,2));await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
