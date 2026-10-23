const assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8114';
const out=path.resolve('build/evidence/performance-settings');
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:process.env.CHROME_PATH||'C:/Program Files/Google/Chrome/Application/chrome.exe',args:['--force_high_performance_gpu']});
 const report={browser:await browser.version(),backends:[]};
 try {for(const backend of ['webgpu','webgl2']) {
  const page=await browser.newPage({viewport:{width:1280,height:800},deviceScaleFactor:1}),errors=[];
  page.setDefaultTimeout(180000);
  page.on('pageerror',e=>errors.push(String(e)));
  page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
  const ready=async()=>{await page.waitForFunction(()=>window.racingReady||window.racingCarError);assert(!await page.evaluate(()=>window.racingCarError));};
  await page.goto(`${base}/?renderer=${backend}`);await ready();
  assert.equal(await page.evaluate(()=>racingBackend),backend);
  const entry={backend,errors,variants:[],build:await page.evaluate(async()=>await(await fetch('build-info.json')).json())};
  entry.wasmSha256=await page.evaluate(async()=>Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',await(await fetch('racing.wasm')).arrayBuffer())),b=>b.toString(16).padStart(2,'0')).join(''));
  await page.locator('#performance-open').click();
  for(const preset of [2,1,0]) {
   await page.locator('#quality-preset').selectOption(String(preset));
   await page.waitForFunction(p=>window.racingQualityApplied?.preset===p,preset);
   const expected=preset===0?75:100;
   await page.waitForFunction(scale=>Module.canvas.width===Math.floor(1280*scale/100),expected);
   await page.waitForTimeout(1500);
   await page.locator('#measurement-reset').click();await page.waitForTimeout(4500);
   const state=await page.evaluate(()=>({quality:racingRenderState,timing:racingFrameTimes,width:Module.canvas.width,height:Module.canvas.height}));
   assert.equal(state.quality.maxShadowMaps,backend==='webgl2'?0:[0,1,4][preset]);
   assert.equal(state.quality.msaaSamples,preset===2?4:1);
   assert(state.timing.frames>5 && state.timing.meanMs>0 && state.timing.p95Ms>=state.timing.meanMs*.5);
   entry.variants.push({preset,...state});
  }
  await page.locator('#resolution-scale').fill('50');await page.locator('#resolution-scale').dispatchEvent('change');
  await page.waitForFunction(()=>Module.canvas.width===640 && Module.canvas.height===400);
  assert.equal(await page.evaluate(()=>getComputedStyle(Module.canvas).width),'1280px');
  assert.equal(await page.evaluate(()=>Module._race_quality(99,1)),0);
  await page.waitForFunction(()=>window.racingFrameTimes?.frames>10);
  await page.screenshot({path:path.join(out,`${backend}-settings.png`)});
  await page.reload();await ready();await page.waitForFunction(()=>Module.canvas.width===640);
  assert.deepEqual(await page.evaluate(()=>racingQualitySettings),{preset:0,scale:50});
  await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);
  await page.keyboard.down('w');await page.waitForFunction(()=>racingState.speed>15);await page.keyboard.up('w');
  await page.locator('#performance-open').click();await page.waitForFunction(()=>racingState.paused);
  const elapsed=await page.evaluate(()=>racingState.elapsed);
  await page.keyboard.press('Escape');assert(!await page.locator('#performance-dialog').isVisible());
  await page.waitForTimeout(250);assert.equal(await page.evaluate(()=>racingState.elapsed),elapsed);
  await page.locator('#resume').click();await page.waitForFunction(()=>!racingState.paused);
  await page.keyboard.down('w');await page.waitForFunction(()=>racingState.elapsed>0.5);await page.keyboard.up('w');
  await page.locator('#performance-open').click();
  await page.setViewportSize({width:390,height:844});
  await page.waitForFunction(()=>Module.canvas.width===195);
  await page.waitForFunction(()=>window.racingFrameTimes?.frames>10);
  await page.screenshot({path:path.join(out,`${backend}-narrow.png`)});
  assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
  const box=await page.locator('#performance-dialog').boundingBox();assert(box.x>=0&&box.x+box.width<=390);
  // Corrupt storage falls back without blocking the game; track changes share settings.
  await page.evaluate(()=>localStorage.setItem('neon-ascension.performance.v1','{"preset":99,"scale":null}'));
  await page.goto(`${base}/?renderer=${backend}&track=desert`);await ready();
  assert.deepEqual(await page.evaluate(()=>racingQualitySettings),{preset:1,scale:100});
  assert.deepEqual(errors,[]);report.backends.push(entry);
  await fs.writeFile(path.join(out,'report.json'),JSON.stringify(report,null,2));
  console.log(backend,'performance settings passed');await page.close();
 }}catch(e){report.failure=String(e.stack||e);throw e;}
 finally{await fs.writeFile(path.join(out,'report.json'),JSON.stringify(report,null,2));await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
