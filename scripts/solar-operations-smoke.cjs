const {chromium}=require('playwright');
const assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path'),crypto=require('node:crypto');
(async()=>{
 const out=path.resolve('build/evidence/solar-operations');await fs.mkdir(out,{recursive:true});
 const base=process.env.DEMO_URL||'http://127.0.0.1:8117';
 const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files/Google/Chrome/Application/chrome.exe'}),report={};
 try{for(const backend of ['webgpu','webgl2']){
  const page=await browser.newPage({viewport:{width:1600,height:1000}}),errors=[];
  page.on('pageerror',e=>errors.push(String(e)));page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
  const ready=async()=>{await page.waitForFunction(()=>window.racingReady||window.racingCarError,null,{timeout:180000});assert(!await page.evaluate(()=>window.racingCarError));assert.equal(await page.evaluate(()=>racingBackend),backend);};
  await page.goto(`${base}/?track=desert&renderer=${backend}&operationsReview=1`);await ready();
  const assets=await page.evaluate(()=>racingDesertAssets);assert.equal(Object.keys(assets).length,20);
  const record=JSON.parse(await fs.readFile('assets/runtime/models/desert-solar/solar-operations/solar-operations.asset.json','utf8'));
  assert.equal(Object.values(assets).find(a=>a.name==='solar-operations').sha256,record.sha256);
  for(const [name,view] of [['operations',192],['sand',0],['canyon',36]]){
   await page.evaluate(v=>window.racingDesertReview=v,view);await page.waitForTimeout(1000);
   await page.screenshot({path:path.join(out,`${backend}-${name}.png`),style:'header,main,footer,.vignette{visibility:hidden!important}'});
  }
  await page.goto(`${base}/?track=desert&renderer=${backend}&mode=trial`);await ready();
  await page.screenshot({path:path.join(out,`${backend}-garage.png`)});
  await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);
  await page.keyboard.down('w');await page.waitForFunction(()=>racingState.speed>35);await page.keyboard.up('w');
  await page.screenshot({path:path.join(out,`${backend}-driving.png`)});
  await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
  assert.deepEqual(errors,[]);
  const wasm=Buffer.from(await(await fetch(`${base}/racing.wasm`)).arrayBuffer());const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
  assert.equal(sha(wasm),sha(await fs.readFile('build/web/site/racing.wasm')));
  report[backend]={asset:record.sha256,triangles:record.triangles,wasm:sha(wasm),errors};await page.close();console.log(backend,'PASS');
 }}finally{await browser.close();await fs.writeFile(path.join(out,'report.json'),JSON.stringify(report,null,2));}
})().catch(e=>{console.error(e);process.exitCode=1;});
