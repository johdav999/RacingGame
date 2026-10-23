const assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path'),crypto=require('node:crypto');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8116',out=path.resolve('build/evidence/desert-race-kit');
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files/Google/Chrome/Application/chrome.exe',args:['--force-high-performance-gpu']});
 const report={browser:await browser.version(),backends:[]};
 try{for(const backend of ['webgpu','webgl2']){
  const page=await browser.newPage({viewport:{width:1440,height:900}}),errors=[];page.setDefaultTimeout(180000);
  page.on('pageerror',e=>errors.push(String(e)));page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
  const ready=async()=>{await page.waitForFunction(()=>window.racingReady||window.racingCarError);assert(!await page.evaluate(()=>window.racingCarError));};
  await page.goto(`${base}/?renderer=${backend}&track=desert`);await ready();
  const r=await page.evaluate(async()=>({backend:racingBackend,assets:racingDesertAssets,placements:racingDesertRacePlacements,build:await(await fetch('build-info.json')).json()}));
  assert.equal(r.backend,backend);assert.equal(Object.keys(r.assets).length,20);
  assert.equal(r.placements.filter(p=>p.model===0).length,592);assert.equal(r.placements.filter(p=>p.model===1).length,12);
  r.wasmSha256=crypto.createHash('sha256').update(Buffer.from(await(await fetch(`${base}/racing.wasm`)).arrayBuffer())).digest('hex');
  await page.locator('#car').click();await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);
  await page.waitForTimeout(3000);await page.screenshot({path:path.join(out,`${backend}-start.png`)});
  await page.keyboard.down('w');await page.waitForFunction(()=>racingState.speed>40);await page.waitForTimeout(800);await page.keyboard.up('w');
  await page.screenshot({path:path.join(out,`${backend}-driving.png`)});
  await page.keyboard.press('r');await page.waitForFunction(()=>racingState.recoveries===1);
  await page.evaluate(()=>resetRacingMeasurements());await page.waitForTimeout(12000);
  r.frameTimes=await page.evaluate(()=>racingFrameTimes);r.render=await page.evaluate(()=>racingPerformance);assert(r.frameTimes.frames>10);
  await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
  await page.goto(`${base}/?renderer=${backend}&track=desert&desertReview=0`);await ready();
  for(const i of [0,16,32,64,96,144]){await page.evaluate(i=>window.racingDesertReview=i,i);await page.waitForTimeout(800);await page.screenshot({path:path.join(out,`${backend}-gate-${i}.png`),style:'header,main,footer,.vignette{visibility:hidden!important}'});}
  assert.deepEqual(errors,[]);r.errors=errors;report.backends.push(r);await fs.writeFile(path.join(out,'report.json'),JSON.stringify(report,null,2));console.log(backend,'PASS',r.frameTimes);await page.close();
 }}catch(e){report.failure=String(e.stack||e);throw e;}finally{await fs.writeFile(path.join(out,'report.json'),JSON.stringify(report,null,2));await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
