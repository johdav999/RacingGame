const assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path'),crypto=require('node:crypto');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8114',out=path.resolve('build/evidence/racing-ai');
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 const report={backends:[],errors:[],wasmSha256:crypto.createHash('sha256').update(await fs.readFile('build/ai-site/racing.wasm')).digest('hex')};
 try{
  for(const backend of ['webgpu','webgl2']){
   const context=await browser.newContext({viewport:{width:1440,height:900}}),page=await context.newPage();page.setDefaultTimeout(120000);
   page.on('pageerror',e=>report.errors.push(String(e)));page.on('console',m=>{if(m.type()==='error')report.errors.push(m.text());});
   const ready=()=>page.waitForFunction(()=>window.racingReady&&window.racingAi&&window.racingState);
   await page.goto(`${base}/?renderer=${backend}&track=desert`);await ready();
   assert.equal(await page.evaluate(()=>racingBackend),backend);
   const served=await page.evaluate(async()=>Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',await(await fetch('racing.wasm')).arrayBuffer())),v=>v.toString(16).padStart(2,'0')).join(''));
   assert.equal(served,report.wasmSha256);
   const result={backend,checks:[],samples:[]};report.backends.push(result);
   for(const difficulty of [0,2,1]){
    await page.locator('#difficulty-select').selectOption(String(difficulty));
    await page.waitForFunction(d=>racingAi.difficulty===d&&localStorage.getItem('neon-ascension.difficulty.v1')===String(d),difficulty);
   }
   await page.locator('#difficulty-select').selectOption('2');await page.waitForFunction(()=>racingAi.difficulty===2);
   await page.reload();await ready();assert.equal(await page.locator('#difficulty-select').inputValue(),'2');
   result.checks.push('three levels selectable; accepted setting persists across reload');
   await page.locator('#mode-select').selectOption('trial');await page.waitForFunction(()=>racingTrial.active);
   assert(await page.locator('#difficulty-select').isDisabled());
   await page.locator('#mode-select').selectOption('race');await page.waitForFunction(()=>!racingTrial.active);
   await page.waitForFunction(()=>!document.getElementById('difficulty-select').disabled);
   result.checks.push('solo time trial disables difficulty and preserves preference');
   await page.waitForFunction(()=>document.getElementById('start').textContent.includes('ENTER CIRCUIT'));
   await page.screenshot({path:path.join(out,`${backend}-garage.png`)});
   await page.setViewportSize({width:390,height:844});
   await page.locator('#difficulty-select').scrollIntoViewIfNeeded();
   assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
   await page.screenshot({path:path.join(out,`${backend}-narrow.png`)});
   await page.setViewportSize({width:1440,height:900});
   await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);
   await page.evaluate(()=>Module._race_action(40));await page.waitForTimeout(100);
   assert.equal(await page.evaluate(()=>racingAi.difficulty),2,'Mid-race changes rejected');
   await page.evaluate(()=>{window.aiSamples=[];window.aiSampler=setInterval(()=>{
    if(racingState.phase===2&&!racingState.paused)aiSamples.push({time:racingState.elapsed,ai:structuredClone(racingAi.racers),standings:structuredClone(racingStandings)});
   },250);});
   await page.waitForTimeout(22000);
   await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
   const paused=await page.evaluate(()=>JSON.stringify(racingAi));await page.waitForTimeout(400);assert.equal(await page.evaluate(()=>JSON.stringify(racingAi)),paused);
   await page.locator('#resume').click();await page.waitForFunction(()=>!racingState.paused);
   result.samples=await page.evaluate(()=>{clearInterval(aiSampler);return aiSamples;});
   assert(result.samples.length>20);
   const all=result.samples.flatMap(s=>s.ai.slice(1));
   assert(all.some(s=>s.brake>0)&&all.some(s=>s.passing>=0),'Live braking and passing decisions');
   assert(all.every(s=>Math.abs(s.lane)<=5.01&&Number.isFinite(s.targetSpeed)));
   assert(result.samples.at(-1).standings.some(s=>s.id&&s.gate>1),'AI advances physical checkpoints');
   await page.screenshot({path:path.join(out,`${backend}-racing.png`)});
   result.checks.push('live corner braking, passing decisions, bounded lanes, physical checkpoints, pause/resume, difficulty locked');
   await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
   // Return through the existing host command, then use the actual track selector.
   await page.evaluate(()=>Module._race_action(7));await page.waitForFunction(()=>racingState.phase===0);
   await page.locator('#track-select').selectOption('kairox');await ready();
   assert.equal(await page.locator('#difficulty-select').inputValue(),'2');
   await page.locator('#difficulty-select').selectOption('0');await page.waitForFunction(()=>racingAi.difficulty===0);
   await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);await page.waitForTimeout(5000);
   assert(await page.evaluate(()=>racingAi.racers.slice(1).every(r=>r.targetSpeed>=0&&r.targetSpeed<=63.1)&&racingAi.racers.some(r=>r.targetSpeed>0)));
   result.checks.push('track switch retains setting; Easy controls Kairox rivals');
   await context.close();
  }
  assert.deepEqual(report.errors,[]);
 }catch(e){report.failure=String(e.stack||e);throw e;}
 finally{await fs.writeFile(path.join(out,'report.json'),JSON.stringify(report,null,2));await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
