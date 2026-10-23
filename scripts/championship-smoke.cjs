// Real rendered races plus explicitly labelled saved-profile fixtures for later leagues.
const assert=require('node:assert/strict');
const fs=require('node:fs/promises');
const path=require('node:path');
const crypto=require('node:crypto');
const {execFileSync}=require('node:child_process');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8135';
const out=path.resolve(process.env.EVIDENCE_DIR||'build/evidence/championship');
const key='neon-ascension.career.v1',origin=new URL(base).origin;
const fixtures=JSON.parse(execFileSync(path.resolve('build/native/racing_championship.exe'),['fixtures'],{encoding:'utf8'}));
const profile=value=>({cookies:[],origins:[{origin,localStorage:[{name:key,value},{name:"neon-ascension.difficulty.v1",value:"1"}]}]});
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:process.env.BROWSER_EXECUTABLE||'C:/Program Files/Google/Chrome/Application/chrome.exe',args:['--disable-background-timer-throttling','--disable-renderer-backgrounding']});
 const wasmSha256=crypto.createHash('sha256').update(await fs.readFile(path.join(process.env.SITE_DIR||'build/web/site','racing.wasm'))).digest('hex');
 const report={browser:await browser.version(),wasmSha256,backends:{},errors:[]};
 try{
  for(const backend of (process.env.BACKENDS||'webgpu,webgl2').split(',')){
   const result=report.backends[backend]={checks:[],fixtures:[]};
   let context,page;
   const ready=async()=>{
    try{await page.waitForFunction(()=>window.racingReady&&window.racingState?.championship,{},{timeout:120000});}
    catch(error){
      result.loadingFailure=await page.evaluate(()=>({status:document.getElementById('status')?.textContent,
        ready:window.racingReady,backend:window.racingBackend,carError:window.racingCarError,audioError:window.racingAudioError,
        state:window.racingState,url:location.href}));
      await page.screenshot({path:path.join(out,`${backend}-loading-failure.png`)});throw error;
    }
    assert.equal(await page.evaluate(()=>racingBackend),backend);
    await page.evaluate(()=>Module._race_quality(0,50));
   };
   const open=async save=>{
    if(context){await context.close();await new Promise(resolve=>setTimeout(resolve,250));}
    context=await browser.newContext({viewport:{width:1440,height:900},storageState:save?profile(save):undefined});
    page=await context.newPage();page.on('pageerror',e=>report.errors.push(String(e)));
    page.on('console',m=>{if(m.type()==='error')report.errors.push({text:m.text(),location:m.location()});});
    page.on('requestfailed',request=>(report.requestFailures||=[]).push({url:request.url(),error:request.failure()?.errorText}));
    await page.goto(`${base}/?renderer=${backend}`);await ready();
   };
   const shot=name=>page.screenshot({path:path.join(out,`${backend}-${name}.png`)});
   await open('NA2 1000 0 0 3 3 3 3 3 3 0');
   const servedHash=await page.evaluate(async()=>Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',await(await fetch('racing.wasm')).arrayBuffer())),v=>v.toString(16).padStart(2,'0')).join(''));
   assert.equal(servedHash,wasmSha256,'Exact generated Wasm is served');
   const glueHash=crypto.createHash('sha256').update(await fs.readFile(path.join(process.env.SITE_DIR||'build/web/site','racing.js'))).digest('hex');
   const servedGlue=await page.evaluate(async()=>Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',await(await fetch('racing.js')).arrayBuffer())),v=>v.toString(16).padStart(2,'0')).join(''));
   assert.equal(servedGlue,glueHash,'Exact generated JavaScript glue is served');result.glueSha256=glueHash;
   result.build=await page.evaluate(async()=>await(await fetch('build-info.json')).json());
   assert.equal(result.build.developmentEngine,false);
   assert.equal(await page.evaluate(()=>racingState.career.credits),1000);
   assert.equal(await page.evaluate(()=>racingLeague.unlocked),0);
   assert((await page.evaluate(k=>localStorage.getItem(k),key)).startsWith('NA5 '));
   await page.locator('#league-open').click();
   assert(await page.locator('#league-tier-4').isDisabled());
   await page.evaluate(()=>Module._race_action(54));await page.waitForTimeout(150);
   assert.equal(await page.evaluate(()=>racingLeague.league),-1);
   await page.locator('#league-tier-0').click();await page.waitForFunction(()=>racingLeague.league===0);
   await shot('street-season');
   await page.setViewportSize({width:390,height:844});
   await page.locator('#league-continue').scrollIntoViewIfNeeded();await shot('narrow-season');
   assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
   assert(await page.evaluate(()=>document.getElementById('league-dialog').getBoundingClientRect().width<=innerWidth));
   await page.setViewportSize({width:1440,height:900});
   await page.locator('#league-continue').click();await page.waitForFunction(()=>racingState.phase===2&&racingLeague.racing);
   assert.equal(await page.evaluate(()=>document.activeElement.id),'viewport','League start focuses driving input');
   assert.equal(await page.evaluate(()=>racingAi.difficulty),0);
   assert.equal(await page.evaluate(()=>localStorage.getItem('neon-ascension.difficulty.v1')),'1','League difficulty does not overwrite quick-race preference');
   assert.equal(await page.evaluate(()=>racingState.combat.charge),100,'Street equipment cap');
   await page.evaluate(()=>Module._race_action(54));
   assert.equal(await page.evaluate(()=>racingLeague.league),0);
   // Restart and abandon retain the uncompleted round and all owned equipment.
   await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
   await page.locator('#pause-panel .restart').click();await page.waitForFunction(()=>racingState.phase===1&&!racingState.paused);
   assert.equal(await page.evaluate(()=>racingLeague.completed),0);
   await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
   await page.locator('#pause-panel .garage-button').click();await page.waitForFunction(()=>racingState.phase===0);
   assert.equal(await page.evaluate(()=>racingState.career.credits),1000);
   assert.deepEqual(await page.evaluate(()=>racingState.career.vehicles),[[3,3,3],[3,3,3]]);
   // Physical race: no fixtures/teleports/result injection. Browser input drives
   // the ordinary physics body through each ordered checkpoint.
   await page.locator('#hover').click();await page.waitForFunction(()=>racingState.chassis===1);
   await page.locator('#league-open').click();await page.locator('#league-continue').click();
   await page.waitForFunction(()=>racingState.phase===2);
   await page.evaluate(()=>{
    window.leaguePilot=setInterval(()=>{
     const s=racingState;if(s.phase!==2||s.paused)return;
     let index=0,best=Infinity;
     for(let i=0;i<racingTrack.length;i++){const p=racingTrack[i],d=(p[0]-s.x)**2+(p[1]-s.z)**2;if(d<best){best=d;index=i;}}
     const target=racingTrack[(index+6)%racingTrack.length];
     let angle=Math.atan2(target[0]-s.x,target[1]-s.z)-s.heading;angle=Math.atan2(Math.sin(angle),Math.cos(angle));
     Module._race_input(101,900,Math.max(-1,Math.min(1,-angle*2)));
     Module._race_input(100,901,s.speed<65?1:.1);Module._race_input(102,902,s.speed>76?.5:0);
     Module._race_input(105,903,1);
    },25);
   });
   console.log(backend+' physical league race started');
   await page.waitForFunction(()=>racingState.phase===3&&racingState.classificationFinal&&racingLeague.completed===1,{},{timeout:360000});
   await page.evaluate(()=>{clearInterval(leaguePilot);Module._race_cancel();});
   const finish=await page.evaluate(()=>({state:racingState,league:racingLeague,save:localStorage.getItem('neon-ascension.career.v1')}));
   const you=finish.state.standings.find(r=>r.id===0),score=[0,15,12,10,8,6,4][you.position];
   assert.equal(finish.league.standings.find(r=>r.id===0).points,score);
   assert.equal(finish.state.career.completedRaces,1);assert.equal(finish.state.career.credits,1000+[0,1200,900,700,500,400,300][you.position]);
   await page.waitForFunction(expected=>document.getElementById('finish-points-0').textContent===String(expected),score);
   assert.equal(await page.locator('#finish-points-0').textContent(),String(score));
   await shot('earned-round-result');await page.waitForTimeout(1200);
   assert.equal(await page.evaluate(k=>localStorage.getItem(k),key),finish.save,'No duplicate award on finished ticks');
   result.physical={playerPosition:you.position,points:score,completedRaces:1,credits:finish.state.career.credits};
   await page.locator('#league-results-open').click();await page.waitForFunction(()=>racingState.phase===0&&document.getElementById('league-dialog').open);
   await shot('earned-standings');
   // Reload using real saved bytes; then travel to the next scheduled scene.
   await page.reload();await ready();assert.equal(await page.evaluate(()=>racingLeague.completed),1);
   await page.locator('#league-open').click();await page.locator('#league-continue').click();
   await page.waitForURL(/track=desert/);await ready();
   assert.equal(await page.evaluate(()=>racingLeague.completed),1);
   assert(await page.locator('#league-dialog').evaluate(el=>el.open));
   await page.locator('#league-continue').click();await page.waitForFunction(()=>racingState.phase===2&&racingLeague.racing);
   assert.equal(await page.evaluate(()=>racingLeague.completed),1,'Loading/starting does not score');
   await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
   await page.locator('#pause-panel .garage-button').click();await page.waitForFunction(()=>racingState.phase===0);
   result.checks.push('legacy wallet/upgrades preserved in NA5','locked league command rejects','narrow career panel','league difficulty/equipment ceiling','restart/abandon retain unscored round','real physical race scores exact final result once','reload and Kairox-to-desert scene travel preserve season');
   console.log(backend+' physical race, standings and track travel passed');
   // Controlled profile fixtures exercise later-career presentation and guards.
   for(const name of ['qualified','finalLost','promoted','failedSeason','masterWon','masterLastRound']){
    await open(fixtures[name]);await page.locator('#league-open').click();
    const c=await page.evaluate(()=>racingLeague);
    if(name==='qualified'){assert.equal(c.status,2);assert.equal(c.completed,4);assert.match(await page.locator('#league-message').textContent(),/Final qualified/);}
    if(name==='finalLost'){assert.equal(c.status,4);assert.equal(c.finalAttempts,1);assert.equal(c.unlocked,0);assert(!(await page.locator('#league-continue').isHidden()));}
    if(name==='promoted'){assert.equal(c.status,5);assert.equal(c.unlocked,1);assert(!(await page.locator('#league-tier-1').isDisabled()));assert(await page.locator('#league-tier-2').isDisabled());}
    if(name==='failedSeason'){assert.equal(c.status,3);assert(await page.locator('#league-continue').isHidden());}
    if(name==='masterWon'){assert.equal(c.trophies,31);assert.equal(c.status,5);assert.match(await page.locator('#league-message').textContent(),/MASTER CHAMPION/);}
    if(name==='masterLastRound'){
     assert.equal(c.league,4);assert.equal(c.completed,5);assert.equal(c.rounds,6);
     await page.locator('#league-continue').click();await page.waitForURL(/track=desert/);await ready();
     await page.locator('#league-continue').click();await page.waitForFunction(()=>racingState.phase===2&&racingLeague.racing);
     assert.equal(await page.evaluate(()=>racingAi.difficulty),2);assert.equal(await page.evaluate(()=>racingState.career.capacity),175);
     await shot('master-race-fixture');
    } else await shot('fixture-'+name);
    result.fixtures.push(name);
   }
   // Corrupt championship and storage failure preserve originals explicitly.
   const invalid=fixtures.nextRound.replace('LC1','LC9');await open(invalid);
   assert.equal(await page.evaluate(k=>localStorage.getItem(k),key),invalid);
   assert.equal(await page.evaluate(()=>racingLeague.league),-1);assert.equal(await page.evaluate(()=>racingState.career.saved),false);
   await open(fixtures.nextRound);
   await page.evaluate(()=>{Storage.prototype.setItem=function(){throw new DOMException('Storage full','QuotaExceededError');};Module._race_action(3);});
   await page.waitForFunction(()=>!racingState.career.saved);
   await page.locator('#league-open').click();assert(await page.locator('#league-continue').isDisabled());
   result.checks.push('qualified final, failed final retry, promotion, failed season and Master fixtures','corrupt championship preserved','storage failure blocks lossy track travel');
   await context.close();await fs.writeFile(path.join(out,'results.json'),JSON.stringify(report,null,2));console.log(backend+' championship passed');
  }
  assert.deepEqual(report.errors,[]);
 }catch(error){report.failure=String(error.stack||error);throw error;}
 finally{await fs.writeFile(path.join(out,'results.json'),JSON.stringify(report,null,2));await browser.close();}
})().catch(error=>{console.error(error);process.exitCode=1;});
