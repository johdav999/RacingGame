// Focused garage, installed-upgrade and durable-save acceptance. Boundary cases
// use explicit saved-profile fixtures; earned-money coverage is in prototype-smoke.
const assert=require('node:assert/strict');
const fs=require('node:fs/promises');
const path=require('node:path');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8090';
const out=path.resolve(process.env.EVIDENCE_DIR||'build/evidence/progression-ui');
const key='neon-ascension.career.v1';
const origin=new URL(base).origin;
const profile=value=>({cookies:[],origins:[{origin,localStorage:[{name:key,value}]}]});
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:process.env.BROWSER_EXECUTABLE||'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 const report={browser:await browser.version(),backends:{},errors:[]};
 try{
  for(const backend of ['webgpu','webgl2']){
   const result=report.backends[backend]={checks:[],vehicles:[]};
   const context=await browser.newContext({viewport:{width:1440,height:900},storageState:profile('NA1 20000 12 0 0 0 0 0 0 0')});
   function observe(page){page.on('pageerror',e=>report.errors.push(String(e)));page.on('console',m=>{if(m.type()==='error')report.errors.push(m.text());});}
   let page=await context.newPage();observe(page);
   async function ready(){await page.waitForFunction(()=>window.racingReady&&window.racingState,{},{timeout:60000});assert.equal(await page.evaluate(()=>racingBackend),backend);}
   await page.goto(`${base}/?renderer=${backend}`);await ready();
   for(const [kind,name] of [[0,'car'],[1,'hover']]){
    await page.locator(kind?'#hover':'#car').click();await page.waitForFunction(k=>racingState.kind===k,kind);
    for(let rank=0;rank<3;rank++)for(let system=0;system<3;system++){
     const before=await page.evaluate(()=>racingState.career);
     await page.locator(`#buy-${system}`).click();
     await page.waitForFunction(({kind,system,rank})=>racingState.career.vehicles[kind][system]===rank+1,{kind,system,rank});
     const after=await page.evaluate(()=>racingState.career);
     assert.equal(after.credits,before.credits-before.prices[system]);
     assert.deepEqual(after.vehicles[1-kind],before.vehicles[1-kind],'Other vehicle stays unchanged');
    }
    await page.waitForFunction(()=>[0,1,2].every(i=>document.getElementById(`buy-${i}`).disabled));
    for(let system=0;system<3;system++)assert(await page.locator(`#buy-${system}`).isDisabled());
    const upgraded=await page.evaluate(()=>racingState.career);
    await page.evaluate(()=>{Module._race_action(20);Module._race_action(21);Module._race_action(22);});
    await page.waitForTimeout(150);assert.equal(await page.evaluate(()=>racingState.career.credits),upgraded.credits,'Native max-rank guard rejects repeated commands');
    assert(Math.abs(upgraded.drive-1.3)<.001&&upgraded.speed===6&&Math.abs(upgraded.interval-.5)<.001&&upgraded.capacity===175&&upgraded.recharge===27);
    await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);
    assert.equal(await page.evaluate(()=>racingState.combat.charge),175);
    await page.keyboard.down('w');await page.keyboard.down('f');await page.keyboard.down('e');
    await page.waitForFunction(()=>racingState.speed>25&&racingState.combat.shots>=3&&racingState.combat.shield);
    const running=await page.evaluate(()=>racingState);assert(running.combat.cooldown<=.501&&running.combat.charge<175);
    await page.screenshot({path:path.join(out,`${backend}-${name}-upgraded-combat.png`)});
    await page.keyboard.up('w');await page.keyboard.up('f');await page.keyboard.up('e');
    await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
    const before=await page.evaluate(()=>racingState.career);
    await page.evaluate(()=>Module._race_action(20));await page.waitForTimeout(150);
    assert.equal(await page.evaluate(()=>racingState.career.credits),before.credits,'No upgrade purchases during paused races');
    await page.locator('#pause-panel .garage-button').click();await page.waitForFunction(()=>racingState.phase===0);
    assert.equal(await page.evaluate(()=>racingState.career.completedRaces),12,'Abandoning a race never grants a payout');
    result.vehicles.push({name,upgraded,running});
   }
   await page.setViewportSize({width:390,height:844});
   await page.locator('#buy-2').scrollIntoViewIfNeeded();await page.waitForTimeout(150);
   await page.screenshot({path:path.join(out,`${backend}-narrow-max-garage.png`)});
   assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
   const start=await page.locator('#start').boundingBox();assert(start.y>=0&&start.y+start.height<844,'Start stays on screen while garage scrolls');
   const saved=await page.evaluate(()=>racingState.career);
   const state=await context.storageState();await context.close();
   // A new browser context simulates a later session with only persisted storage.
   const reopened=await browser.newContext({storageState:state});page=await reopened.newPage();observe(page);
   await page.goto(`${base}/?renderer=${backend}`);await ready();
   const restored=await page.evaluate(()=>racingState.career);
   for(const field of ['credits','completedRaces','selected','vehicles'])assert.deepEqual(restored[field],saved[field]);
   assert.equal(restored.reward,0);assert(restored.saved);
   result.checks.push('all 18 upgrades bought at exact prices','max-tier native purchase guard','separate car and hover upgrades','installed cannon/shield in both moving chassis','no payout for abandoned races','390px scroll and fixed start','new-session save restoration');
   // Switching renderer on the same origin uses the same career.
   const other=backend==='webgpu'?'webgl2':'webgpu';
   await page.goto(`${base}/?renderer=${other}`);await page.waitForFunction(()=>window.racingReady&&window.racingState,{},{timeout:60000});
   assert.equal(await page.evaluate(()=>racingBackend),other);
   assert.deepEqual(await page.evaluate(()=>racingState.career.vehicles),saved.vehicles);
   assert.equal(await page.evaluate(()=>racingState.career.credits),saved.credits);
   result.checks.push('renderer switch preserves career');
   const peer=await reopened.newPage();
   await peer.route('**/storage-peer',route=>route.fulfill({contentType:'text/html',body:'<title>Save concurrency fixture</title>'}));
   await peer.goto(`${base}/storage-peer`);
   const peerSave='NA1 1234 13 0 0 0 0 0 0 0';
   await peer.evaluate(({key,value})=>localStorage.setItem(key,value),{key,value:peerSave});
   await page.bringToFront();await page.waitForFunction(()=>!racingState.career.saved);
   await page.locator('#car').click();await page.waitForFunction(()=>racingState.kind===0);
   assert.equal(await page.evaluate(k=>localStorage.getItem(k),key),peerSave,'Old tab must not overwrite a newer save');
   await page.waitForFunction(()=>document.getElementById('save-status').textContent.includes('another tab'));
   assert.match(await page.locator('#save-status').textContent(),/another tab/);
   result.checks.push('newer tab save is protected from stale writes');await reopened.close();
   for(const fixture of ['NA9 unknown future version','NA1 -500 0 0 0 0 0 0 0 0']){
    const bad=await browser.newContext({storageState:profile(fixture)});page=await bad.newPage();observe(page);
    await page.goto(`${base}/?renderer=${backend}`);await ready();
    assert.equal(await page.evaluate(()=>racingState.career.credits),0);
    assert.equal(await page.evaluate(k=>localStorage.getItem(k),key),fixture,'Unreadable original is preserved');
    assert.equal(await page.evaluate(()=>racingState.career.saved),false);
    assert.match(await page.locator('#save-status').textContent(),/unreadable/);await bad.close();
   }
   result.checks.push('unknown-version and corrupt saves preserved with visible session-only status');
   const denied=await browser.newContext();page=await denied.newPage();observe(page);
   await page.addInitScript(()=>{Storage.prototype.setItem=function(){throw new DOMException('Storage full','QuotaExceededError');};});
   await page.goto(`${base}/?renderer=${backend}`);await ready();
   assert.equal(await page.evaluate(()=>racingState.career.saved),false);
   assert.match(await page.locator('#save-status').textContent(),/session only/);
   assert(await page.locator('#buy-0').isDisabled());
   await page.evaluate(()=>Module._race_action(20));await page.waitForTimeout(150);
   assert.equal(await page.evaluate(()=>racingState.career.credits),0);
   assert.deepEqual(await page.evaluate(()=>racingState.career.vehicles),[[0,0,0],[0,0,0]]);
   result.checks.push('quota failure remains playable and explicitly unsaved','native insufficient-credit guard');await denied.close();
   await fs.writeFile(path.join(out,'results.json'),JSON.stringify(report,null,2));console.log(backend+' progression passed');
  }
  assert.deepEqual(report.errors,[]);
 }catch(error){report.failure=String(error.stack||error);throw error;}
 finally{await fs.writeFile(path.join(out,'results.json'),JSON.stringify(report,null,2));await browser.close();}
})().catch(error=>{console.error(error);process.exitCode=1;});
