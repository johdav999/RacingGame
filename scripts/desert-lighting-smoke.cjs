// Fixed village cameras allow a reproducible lighting comparison without moving
// the user's open game. Also exercise driving and both forced rendering backends.
const assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path'),crypto=require('node:crypto');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8117';
const out=path.resolve(process.env.EVIDENCE_DIR||'build/evidence/desert-lighting/after');
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 const report={browser:await browser.version(),backends:[]};
 try{for(const backend of ['webgpu','webgl2']){
  const page=await browser.newPage({viewport:{width:1440,height:900}}),errors=[];
  page.on('pageerror',e=>errors.push(String(e)));page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
  const ready=async()=>{
   await page.waitForFunction(()=>window.racingReady||window.racingCarError,null,{timeout:180000});
   assert(!await page.evaluate(()=>window.racingCarError));assert.equal(await page.evaluate(()=>racingBackend),backend);
  };
  await page.goto(`${base}/?renderer=${backend}&track=desert&villageReview=0`);await ready();
  const wasm=Buffer.from(await(await fetch(`${base}/racing.wasm`)).arrayBuffer());
  const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
  assert.equal(sha(wasm),sha(await fs.readFile('build/web/site/racing.wasm')));
  for(let village=0;village<2;village++){
   await page.evaluate(v=>window.racingVillageReview=v,village);await page.waitForTimeout(1500);
   await page.screenshot({path:path.join(out,`${backend}-village-${village}.png`),style:'header,main,footer,.vignette{visibility:hidden!important}'});
  }
  await page.goto(`${base}/?renderer=${backend}&track=desert&mode=trial`);await ready();
  await page.locator('#car').click();await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);
  await page.keyboard.down('w');await page.waitForFunction(()=>racingState.speed>35);await page.keyboard.up('w');
  await page.screenshot({path:path.join(out,`${backend}-driving.png`)});
  await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
  assert.deepEqual(errors,[]);
  report.backends.push({backend,wasmSha256:sha(wasm),errors});
  await page.close();console.log(backend,'PASS');
 }}finally{await fs.writeFile(path.join(out,'report.json'),JSON.stringify(report,null,2));await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
