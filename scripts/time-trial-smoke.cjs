const assert=require('node:assert/strict');
const fs=require('node:fs/promises');
const path=require('node:path');
const crypto=require('node:crypto');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8093';
const out=path.resolve(process.env.EVIDENCE_DIR||'build/evidence/time-trial');
const key='neon-ascension.trial.v1.0.0';
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:process.env.BROWSER_EXECUTABLE||'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 const wasmSha256=crypto.createHash('sha256').update(await fs.readFile(path.resolve('build/web/site/racing.wasm'))).digest('hex');
 const report={browser:await browser.version(),wasmSha256,backends:{},errors:[]};
 try {
  for(const backend of ['webgpu','webgl2']) {
   const context=await browser.newContext({viewport:{width:1440,height:900}});
   const page=await context.newPage();page.on('pageerror',e=>report.errors.push(String(e)));
   page.on('console',m=>{if(m.type()==='error')report.errors.push(m.text());});
   const ready=()=>page.waitForFunction(()=>window.racingReady&&window.racingState?.trial,{},{timeout:90000});
   const shot=name=>page.screenshot({path:path.join(out,`${backend}-${name}.png`)});
   await page.goto(`${base}/?renderer=${backend}&mode=trial`);await ready();
   assert.equal(await page.evaluate(()=>racingBackend),backend);
   const build=await page.evaluate(async()=>await(await fetch('build-info.json')).json());
   const servedHash=await page.evaluate(async()=>Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',await(await fetch('racing.wasm')).arrayBuffer())),v=>v.toString(16).padStart(2,'0')).join(''));
   assert.equal(servedHash,wasmSha256,'Served Wasm must match this generated build');
   assert.equal(build.developmentEngine,false);assert.equal(build.engineDirty,false);
   const result=report.backends[backend]={build,checks:[]};
   assert.equal(await page.evaluate(()=>racingState.trial.best),0);
   assert(await page.locator('#garage .watch-best').isDisabled());
   await shot('garage');
   await page.setViewportSize({width:390,height:844});await shot('narrow-garage');
   assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
   await page.setViewportSize({width:1440,height:900});
   await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);
   const career=await page.evaluate(()=>racingState.career);
   await page.evaluate(()=>{
    window.trialPilot=setInterval(()=>{
     const s=racingState;if(s.phase!==2||s.paused||s.trial.replay)return;
     let index=0,best=Infinity;
     for(let i=0;i<racingTrack.length;i++){const p=racingTrack[i],d=(p[0]-s.x)**2+(p[1]-s.z)**2;if(d<best){best=d;index=i;}}
     const target=racingTrack[(index+6)%racingTrack.length];
     let angle=Math.atan2(target[0]-s.x,target[1]-s.z)-s.heading;angle=Math.atan2(Math.sin(angle),Math.cos(angle));
     Module._race_input(101,900,Math.max(-1,Math.min(1,-angle*2)));
     Module._race_input(100,901,s.speed<65?1:.1);Module._race_input(102,902,s.speed>76?.5:0);
     Module._race_input(105,903,1);Module._race_input(106,904,1);
    },35);
   });
   await page.waitForFunction(()=>racingState.trial.splits[0]>0,{},{timeout:90000});await shot('sector');
   await page.waitForFunction(()=>racingState.phase===3,{},{timeout:120000});
   await page.evaluate(()=>{clearInterval(trialPilot);Module._race_cancel();});
   await page.waitForFunction(()=>document.getElementById('result-title').textContent==='NEW PERSONAL BEST.');
   const finish=await page.evaluate(()=>racingState);result.finish=finish;
   assert(finish.trial.clean&&finish.trial.newBest&&finish.trial.best===finish.elapsed);
   assert.equal(finish.lap,1);assert.equal(finish.combat.shots,0);assert(!finish.combat.shield);
   assert.equal(finish.career.credits,career.credits);assert.equal(finish.career.completedRaces,career.completedRaces);
   assert(finish.trial.splits[0]>0&&finish.trial.splits[1]>finish.trial.splits[0]&&finish.trial.splits[2]===finish.elapsed);
   const saved=await page.evaluate(k=>localStorage.getItem(k),key);assert(saved?.startsWith('NATT1 0 0 '));
   await fs.writeFile(path.join(out,`${backend}-best.txt`),saved);
   await shot('personal-best');result.checks.push('physical one-lap finish, all sectors, no combat/payout, saved PB');
   await page.reload();await ready();
   assert.equal(await page.evaluate(()=>racingState.trial.best),finish.trial.best);
   await page.locator('#garage .watch-best').click();await page.waitForFunction(()=>racingState.trial.replay&&racingState.elapsed>3);
   assert(await page.evaluate(()=>racingState.trial.ghost));await shot('replay');
   await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
   const paused=await page.evaluate(()=>racingState);await page.waitForTimeout(350);
   assert.equal(await page.evaluate(()=>racingState.elapsed),paused.elapsed);
   assert.deepEqual(await page.evaluate(()=>[racingTrial.x,racingTrial.y,racingTrial.z]),[paused.trial.x,paused.trial.y,paused.trial.z]);
   await page.locator('#resume').click();await page.waitForFunction(()=>racingState.phase===3,{},{timeout:90000});
   assert.equal(await page.evaluate(()=>racingState.elapsed),finish.elapsed);
   assert.equal(await page.evaluate(k=>localStorage.getItem(k),key),saved);
   result.checks.push('reload restores PB, visible replay, pause freezes poses, exact replay end without overwriting');
   await page.locator('#result .restart').click();await page.waitForFunction(()=>racingState.phase===2&&racingState.elapsed>1);
   assert(await page.evaluate(()=>racingState.trial.ghost&&!racingState.trial.replay));await shot('race-ghost');
   await page.setViewportSize({width:390,height:844});await shot('narrow-ghost');await page.setViewportSize({width:1440,height:900});
   await page.keyboard.press('r');await page.waitForFunction(()=>!racingState.trial.clean);
   assert.equal(await page.evaluate(k=>localStorage.getItem(k),key),saved);
   await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
   await page.locator('#pause-panel .garage-button').click();await page.waitForFunction(()=>racingState.phase===0);
   for(const [button,chassis] of [['hover',1],['bastion',2],['vector',3]]) {
    await page.locator('#'+button).click();
    await page.waitForFunction(c=>racingState.chassis===c&&racingState.trial.best===0&&document.querySelector('#garage .watch-best').disabled,chassis);
    assert(await page.locator('#garage .watch-best').isDisabled());
   }
   await page.locator('#car').click();await page.waitForFunction(()=>racingState.trial.best>0);
   await page.locator('#track-select').selectOption('desert');await ready();assert.equal(await page.evaluate(()=>racingState.trial.best),0);
   await page.locator('#track-select').selectOption('kairox');await ready();assert.equal(await page.evaluate(()=>racingState.trial.best),finish.elapsed);
   await page.locator('#mode-select').selectOption('race');await page.waitForFunction(()=>!racingState.trial.active);
   await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2&&racingState.elapsed>2);
   assert(await page.locator('#standings').isVisible());assert.equal(await page.evaluate(()=>racingState.standings.length),6);
   result.checks.push('retry races non-colliding ghost, recovery preserves PB, chassis/track isolation, return to six-racer mode');
   console.log(`${backend}: lap, save, replay, ghost and mode isolation passed`);
   await context.close();
  }
  // Storage failures use explicit fixtures, separate from the physical-lap evidence.
  for(const failure of ['corrupt','quota','other-tab']) {
   const context=await browser.newContext();const page=await context.newPage();
   if(failure==='corrupt')await page.addInitScript(k=>localStorage.setItem(k,'NATT9 future-format'),key);
   if(failure==='quota')await page.addInitScript(()=>{Storage.prototype.setItem=function(){throw new DOMException('full','QuotaExceededError');};});
   await page.goto(`${base}/?renderer=webgl2&mode=trial`);await page.waitForFunction(()=>window.racingReady&&window.racingState?.trial,{},{timeout:90000});
   if(failure==='corrupt') {
    assert.match(await page.locator('#trial-summary').textContent(),/unreadable/);
    assert.equal(await page.evaluate(k=>localStorage.getItem(k),key),'NATT9 future-format');
   }else if(failure==='quota') {
    await page.evaluate(()=>saveTimeTrial(0,0,'fixture write'));
    await page.waitForFunction(()=>document.getElementById('trial-summary').textContent.includes('session only'));
   }else {
    const peer=await context.newPage();await peer.goto(base+'/');await peer.evaluate(k=>localStorage.setItem(k,'newer-tab-data'),key);await page.bringToFront();
    await page.waitForFunction(()=>document.getElementById('trial-summary').textContent.includes('another tab'));
    await page.evaluate(()=>saveTimeTrial(0,0,'stale write'));assert.equal(await page.evaluate(k=>localStorage.getItem(k),key),'newer-tab-data');
   }
   await context.close();
  }
  report.storageChecks=['corrupt record preserved','quota failure keeps session best','other-tab write protected'];
  assert.deepEqual(report.errors,[]);
  await fs.writeFile(path.join(out,'report.json'),JSON.stringify(report,null,2));console.log('Both renderers and storage checks passed. Report: '+path.join(out,'report.json'));
 }finally{await browser.close();}
})().catch(error=>{console.error(error);process.exitCode=1;});
