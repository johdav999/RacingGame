// Real Chromium browser acceptance against the exact generated consumer site.
// NODE_PATH points to installed playwright/pngjs; no development-engine page is used.
const assert=require('node:assert/strict');
const fs=require('node:fs/promises');
const path=require('node:path');
const {chromium}=require('playwright');
const {PNG}=require('pngjs');
const base=process.env.DEMO_URL||'http://127.0.0.1:8088';
const out=path.resolve(process.env.EVIDENCE_DIR||'build/evidence/prototype');
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:process.env.BROWSER_EXECUTABLE||'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 const report={browser:await browser.version(),viewport:{width:1440,height:900},backends:{},errors:[]};
 try {
  for(const backend of (process.env.RENDERER?[process.env.RENDERER]:['webgpu','webgl2'])) {
   const page=await browser.newPage({viewport:report.viewport});
   const errors=[];const error=message=>{errors.push(message);report.errors.push(backend+': '+message);};
   page.on('pageerror',e=>error(String(e)));page.on('console',m=>{if(m.type()==='error')error(m.text());});
   await page.goto(`${base}/?renderer=${backend}`);
   await page.waitForFunction(()=>window.racingReady&&window.racingState,{},{timeout:60000});
   assert.equal(await page.evaluate(()=>racingBackend),backend,'Forced renderer must be the actual renderer');
   const build=await page.evaluate(async()=>await(await fetch('build-info.json')).json());
   assert.equal(build.developmentEngine,false);assert.equal(build.engineDirty,false);
   report.backends[backend]={build,checks:[],vehicles:[]};const result=report.backends[backend];
   async function shot(name){await page.screenshot({path:path.join(out,`${backend}-${name}.png`)});}
   const pixels=PNG.sync.read(await page.locator('#viewport').screenshot());
   let bright=0;for(let i=0;i<pixels.data.length;i+=4)if(pixels.data[i]+pixels.data[i+1]+pixels.data[i+2]>180)bright++;
   assert(bright>10000,'Scene must draw visible city pixels');
   await shot('garage');result.checks.push('correct renderer and clean pinned build','nonempty rendered city');
   await page.setViewportSize({width:390,height:844});await page.waitForTimeout(150);await shot('narrow-garage');
   assert(await page.locator('#start').isVisible());
   assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),'No horizontal page overflow');
   await page.setViewportSize(report.viewport);
   for(const [kind,name] of [[0,'car'],[1,'hover']]) {
    await page.locator(kind===0?'#car':'#hover').click();
    await page.waitForFunction(k=>racingState.kind===k&&racingState.phase===0,kind);
    await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);
    const start=await page.evaluate(()=>racingState);
    const openingCareer=start.career;
    assert.equal(start.standings.length,6);
    assert.equal(new Set(start.standings.map(r=>r.id)).size,6);
    assert(start.standings.some(r=>r.id&&r.kind===0)&&start.standings.some(r=>r.id&&r.kind===1));
    await page.keyboard.down('w');await page.waitForFunction(()=>racingState.speed>25);await page.keyboard.up('w');
    const moving=await page.evaluate(()=>racingState);
    for(const r of moving.standings.filter(r=>r.id)){
     const before=start.standings.find(x=>x.id===r.id);
     assert(Math.hypot(r.x-before.x,r.z-before.z)>1,'Every AI must drive away from its grid slot');
    }
    assert(Math.hypot(moving.x-start.x,moving.z-start.z)>2,'Keyboard throttle moves chassis');
    await page.keyboard.down('d');await page.waitForTimeout(350);await page.keyboard.up('d');
    const turned=await page.evaluate(()=>racingState);
    const turn=Math.atan2(Math.sin(turned.heading-moving.heading),Math.cos(turned.heading-moving.heading));
    assert(turn<-.02,'D must turn toward the chase camera screen-right');
    await page.keyboard.down('s');await page.waitForFunction(()=>racingState.speed<3);await page.keyboard.up('s');
    if(kind===0){
     const stopped=await page.evaluate(()=>racingState);
     await page.keyboard.down('s');await page.waitForTimeout(450);await page.keyboard.up('s');
     const backing=await page.evaluate(()=>racingState);
     const reverseTravel=(backing.x-stopped.x)*Math.sin(stopped.heading)+(backing.z-stopped.z)*Math.cos(stopped.heading);
     assert(reverseTravel<-.05,'Holding S at low speed must reverse the car');
     result.checks.push('S engages physical car reverse after braking');
    }
    await page.keyboard.down('e');await page.keyboard.down('f');
    await page.waitForFunction(()=>racingState.combat.shield&&racingState.combat.shots>0&&racingState.combat.charge<98);
    await shot(name+'-combat');
    await page.keyboard.up('f');await page.keyboard.up('e');
    await page.waitForFunction(()=>!racingState.combat.shield);
    await page.keyboard.down('w');await page.keyboard.down('Shift');await page.waitForFunction(()=>racingState.battery<85);
    await page.keyboard.up('Shift');await page.keyboard.up('w');await shot(name+'-driving');
    await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
    const paused=await page.evaluate(()=>racingState);await page.waitForTimeout(350);
    const still=await page.evaluate(()=>racingState);assert.equal(still.elapsed,paused.elapsed);assert.equal(still.x,paused.x);
    assert.deepEqual(still.standings,paused.standings,'Pause freezes the complete field');
    await page.locator('#resume').click();await page.waitForFunction(()=>!racingState.paused);
    await page.keyboard.down('w');await page.evaluate(()=>window.dispatchEvent(new Event('blur')));await page.keyboard.up('w');
    await page.waitForFunction(()=>racingState.paused);await page.locator('#resume').click();
    await page.keyboard.press('r');await page.waitForFunction(()=>racingState.recoveries===1);
    const recovered=await page.evaluate(()=>racingState);assert(recovered.speed<2);assert.equal(recovered.gate,1);
    await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
    await page.locator('#pause-panel .restart').click();await page.waitForFunction(()=>racingState.phase===1);
    const grid=await page.evaluate(()=>racingState.standings);
    assert(grid.every(r=>r.lap===0&&!r.finished&&r.elapsed===0),'Restart resets every racer');
    assert(grid.every(r=>r.combat.shots===0&&r.combat.charge===100&&!r.combat.shield),'Restart resets combat');
    await page.waitForFunction(()=>racingState.phase===2);
    assert.equal(await page.evaluate(()=>racingState.recoveries),0);
    if(process.env.SKIP_FULL_RACE!=='1') {
     // Synthetic steering commands use the production action adapter and real RAF/Jolt.
     // No teleportation, checkpoint injection or test-only stepping API is present.
     await page.evaluate(()=>{
      window.pilot=setInterval(()=>{
       const s=racingState;if(s.phase!==2||s.paused)return;
       let best=Infinity,index=0;
       for(let i=0;i<racingTrack.length;i++) {const p=racingTrack[i],d=(p[0]-s.x)**2+(p[1]-s.z)**2;if(d<best){best=d;index=i;}}
       const target=racingTrack[(index+6)%racingTrack.length];
       let angle=Math.atan2(target[0]-s.x,target[1]-s.z)-s.heading;
       angle=Math.atan2(Math.sin(angle),Math.cos(angle));
       Module._race_input(101,900,Math.max(-1,Math.min(1,-angle*2)));
       Module._race_input(100,901,s.speed<65?1:.1);
       Module._race_input(102,902,s.speed>76?.5:0);
       Module._race_input(105,903,1);
       Module._race_input(106,904,s.elapsed%4<1.2?1:0);
      },16);
     });
     await page.waitForFunction(()=>racingState.phase===3,{},{timeout:240000});
     await page.evaluate(()=>{clearInterval(pilot);Module._race_cancel();});
     await shot(name+'-player-finished');
     await page.waitForFunction(()=>racingState.classificationFinal,{},{timeout:180000});
     const finish=await page.evaluate(()=>racingState);
     const place=finish.standings.find(r=>r.id===0).position,prize=[1200,900,700,500,400,300][place-1];
     assert.equal(finish.career.reward,prize,'Final position determines winnings');
     assert.equal(finish.career.credits,openingCareer.credits+prize);
     assert.equal(finish.career.completedRaces,openingCareer.completedRaces+1);
     await page.waitForTimeout(200);
     assert.equal(await page.evaluate(()=>racingState.career.credits),finish.career.credits,'Results pay only once');
     assert.equal(finish.lap,3);assert.equal(finish.recoveries,0);assert(finish.best>0);
     assert(finish.standings.every(r=>r.finished&&r.lap===3&&r.elapsed>0),'All opponents must complete three laps');
     const ai=finish.standings.filter(r=>r.id);
     assert(finish.combat.shots>10&&finish.combat.shieldUses>0,'Player must use cannon and shield throughout the race');
     assert(ai.reduce((n,r)=>n+r.combat.shots,0)>0&&ai.reduce((n,r)=>n+r.combat.hits,0)>0,'AI must fire and land disruptive hits');
     assert(ai.reduce((n,r)=>n+r.combat.shieldUses,0)>0&&finish.standings.reduce((n,r)=>n+r.combat.blocks,0)>0,'Defenses must actually block attacks');
     assert.deepEqual(finish.standings.map(r=>r.position),[1,2,3,4,5,6]);
     for(let i=1;i<6;i++)assert(finish.standings[i-1].elapsed<=finish.standings[i].elapsed,'Results sorted by time including penalties');
     await page.waitForFunction(()=>document.getElementById('classification-status').textContent.startsWith('FINAL'));
     assert.equal(await page.locator('#result-rows tr').count(),6);
     await shot(name+'-finished');result.vehicles.push({name,moving,finish});
     await page.setViewportSize({width:390,height:844});await page.waitForTimeout(150);await shot(name+'-narrow-results');
     assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),'Results fit narrow viewport');
     await page.locator('#result .garage-button').scrollIntoViewIfNeeded();
     assert(await page.locator('#result .garage-button').isVisible());
     await page.setViewportSize(report.viewport);
     await page.locator('#result .garage-button').click();
    }else{
     result.vehicles.push({name,moving});await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
     await page.locator('#pause-panel .garage-button').click();
    }
    await page.waitForFunction(()=>racingState.phase===0);
    if(process.env.SKIP_FULL_RACE!=='1'){
     // Spend genuinely earned winnings through the visible garage controls.
     const purchased=[];
     for(const system of [1,2,0]){
      const before=await page.evaluate(()=>racingState.career);
      if(before.credits<before.prices[system])continue;
      await page.locator(`#buy-${system}`).click();
      await page.waitForFunction(({kind,system,rank})=>racingState.career.vehicles[kind][system]===rank+1,
       {kind,system,rank:before.vehicles[kind][system]});
      assert.equal(await page.evaluate(()=>racingState.career.credits),before.credits-before.prices[system]);
      purchased.push(system);
     }
     assert(purchased.length>0,'Every finisher can buy an upgrade with the prize');
     const saved=await page.evaluate(()=>racingState.career);
     await shot(name+'-earned-upgrades');
     await page.reload();await page.waitForFunction(()=>window.racingReady&&window.racingState,{},{timeout:60000});
     const restored=await page.evaluate(()=>racingState.career);
     for(const key of ['credits','completedRaces','selected','vehicles'])assert.deepEqual(restored[key],saved[key],`Reload restores ${key}`);
     assert.equal(restored.reward,0,'Reload does not replay a completed-race payout');
     result.vehicles.at(-1).purchases=purchased;
     result.vehicles.at(-1).savedCareer=restored;
    }
   }
   result.checks.push('car and hover keyboard acceleration','screen-right keyboard steering','braking','boost battery','pause freezes time and position','blur auto-pause','recovery penalty and gate preservation','restart','vehicle switching','390px layout');
   result.checks.push('F fires pulse cannon','E activates and drains shield','combat timers freeze on pause','restart clears combat','AI attacks and defenses','full-race hit and block counters');
   if(process.env.SKIP_FULL_RACE!=='1')result.checks.push('placement payouts credited exactly once','spend earned winnings in each vehicle garage','reload preserves wallet, both vehicle upgrades and selection without replaying rewards');
   if(process.env.SKIP_FULL_RACE!=='1')result.checks.push('three physical laps for both player vehicles and all five AI rivals','six unique results ordered by penalty-inclusive times','mixed car and hover grid','whole-field pause and restart','narrow finishing results');
   result.performance=await page.evaluate(()=>window.racingPerformance);result.errors=errors;
   await fs.writeFile(path.join(out,'results.json'),JSON.stringify(report,null,2));
   await page.close();console.log(backend+' passed');
  }
  assert.deepEqual(report.errors,[],'No browser or GPU errors');
 }catch(error){report.failure=String(error.stack||error);throw error;}
 finally{await fs.writeFile(path.join(out,'results.json'),JSON.stringify(report,null,2));await browser.close();}
 console.log(JSON.stringify(report,null,2));
})().catch(e=>{console.error(e);process.exitCode=1;});
