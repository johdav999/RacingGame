// Verify the generated site, exact assets, two villages and real driving on both backends.
const assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path'),crypto=require('node:crypto');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8123',out=path.resolve('build/evidence/dune-villages');
const sha=bytes=>crypto.createHash('sha256').update(bytes).digest('hex');
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const manifest=JSON.parse(await fs.readFile('assets/runtime/models/dune-villages/manifest.json','utf8'));
 const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files/Google/Chrome/Application/chrome.exe',args:['--force-high-performance-gpu']});
 const report={browser:await browser.version(),backends:[]};
 try{for(const backend of ['webgpu','webgl2']){
  const page=await browser.newPage({viewport:{width:1440,height:900}}),errors=[],requests=[];
  page.setDefaultTimeout(180000);
  page.on('pageerror',e=>errors.push(String(e)));page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
  page.on('request',r=>{if(r.url().endsWith('.glb'))requests.push(r.url());});
  const ready=async()=>{await page.waitForFunction(()=>window.racingReady||window.racingCarError);assert(!await page.evaluate(()=>window.racingCarError));};
  await page.goto(`${base}/?renderer=${backend}&track=desert`);await ready();
  const r=await page.evaluate(async()=>({backend:racingBackend,assets:racingDesertAssets,placements:racingVillagePlacements,build:await(await fetch('build-info.json')).json()}));
  assert.equal(r.backend,backend);assert.equal(Object.keys(r.assets).length,20);
  assert.equal(r.placements.length,12);
  for(let v=0;v<2;v++){
   const placements=r.placements.filter(p=>p.village===v);assert.equal(placements.length,6);assert.equal(new Set(placements.map(p=>p.model)).size,5);
  }
  for(let i=0;i<5;i++){
   const asset=manifest.assets[i];assert.equal(r.assets[14+i].sha256,asset.sha256);
   const served=Buffer.from(await(await fetch(`${base}/assets/models/dune-villages/${asset.name}/${asset.name}.glb`)).arrayBuffer());
   assert.equal(sha(served),asset.sha256);assert.equal(sha(await fs.readFile(asset.source)),asset.source_sha256);
  }
  assert(!requests.some(url=>/\/(buildings|city-kit|street-kit|track-kit|plaza)\//.test(url)));
  r.wasmSha256=sha(Buffer.from(await(await fetch(`${base}/racing.wasm`)).arrayBuffer()));
  assert.equal(r.wasmSha256,sha(await fs.readFile('build/web/site/racing.wasm')));
  await page.locator('#car').click();await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);
  await page.keyboard.down('w');await page.waitForFunction(()=>racingState.speed>40);await page.keyboard.up('w');
  await page.screenshot({path:path.join(out,`${backend}-driving.png`)});
  await page.keyboard.press('r');await page.waitForFunction(()=>racingState.recoveries===1);
  await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
  await page.goto(`${base}/?renderer=${backend}&track=desert&villageReview=0`);await ready();
  for(let village=0;village<2;village++){
   await page.evaluate(v=>window.racingVillageReview=v,village);await page.waitForTimeout(1000);
   await page.screenshot({path:path.join(out,`${backend}-village-${village}.png`),style:'header,main,footer,.vignette{visibility:hidden!important}'});
  }
  await page.evaluate(()=>resetRacingMeasurements());await page.waitForTimeout(10000);
  r.frameTimes=await page.evaluate(()=>racingFrameTimes);r.render=await page.evaluate(()=>racingPerformance);
  await page.locator('#track-select').selectOption('kairox');await page.waitForURL('**track=kairox');await ready();
  assert.equal(Object.keys(await page.evaluate(()=>racingDesertAssets)).length,0);
  assert.deepEqual(errors,[]);r.errors=errors;report.backends.push(r);
  await fs.writeFile(path.join(out,'report.json'),JSON.stringify(report,null,2));console.log(backend,'PASS');await page.close();
 }}catch(e){report.failure=String(e.stack||e);throw e;}finally{await fs.writeFile(path.join(out,'report.json'),JSON.stringify(report,null,2));await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
