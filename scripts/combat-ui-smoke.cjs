// Focused energy/lifecycle and narrow-HUD checks. Run after the full race suite.
const assert=require('node:assert/strict');
const fs=require('node:fs/promises');
const path=require('node:path');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8089';
const out=path.resolve(process.env.EVIDENCE_DIR||'build/evidence/combat-ui');
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:process.env.BROWSER_EXECUTABLE||'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 const report={backends:[],errors:[]};
 try{
  for(const backend of ['webgpu','webgl2']){
   const page=await browser.newPage({viewport:{width:390,height:844}});
   page.on('pageerror',e=>report.errors.push(String(e)));
   page.on('console',m=>{if(m.type()==='error')report.errors.push(m.text());});
   await page.goto(`${base}/?renderer=${backend}`);
   await page.waitForFunction(()=>window.racingReady&&window.racingState,{},{timeout:60000});
   assert.equal(await page.evaluate(()=>racingBackend),backend);
   await page.locator('#hover').click();await page.waitForFunction(()=>racingState.kind===1);
   await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);
   await page.keyboard.down('e');await page.keyboard.down('f');
   await page.waitForFunction(()=>racingState.combat.shield&&racingState.combat.shots>0);
   await page.waitForFunction(()=>document.getElementById('shield-state').textContent==='SHIELD ACTIVE'&&document.getElementById('weapon-state').textContent.startsWith('CHARGING'));
   await page.screenshot({path:path.join(out,backend+'-narrow-shield.png')});
   const boxes=await page.evaluate(()=>{
    const rect=id=>{const r=document.getElementById(id).getBoundingClientRect();return {x:r.x,y:r.y,right:r.right,bottom:r.bottom};};
    return {combat:rect('combat-panel'),field:rect('standings'),hud:rect('race-hud'),width:innerWidth,overflow:document.documentElement.scrollWidth>innerWidth};
   });
   assert(!boxes.overflow&&boxes.combat.right<=boxes.width);
   assert(boxes.field.right<=boxes.combat.x&&boxes.hud.bottom<=boxes.combat.y,'Narrow combat HUD must not overlap race data');
   await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
   const paused=await page.evaluate(()=>racingState.combat);
   await page.waitForTimeout(300);assert.deepEqual(await page.evaluate(()=>racingState.combat),paused);
   await page.keyboard.up('e');await page.keyboard.up('f');
   await page.locator('#resume').click();await page.waitForFunction(()=>!racingState.paused&&!racingState.combat.shield);
   const released=await page.evaluate(()=>racingState.combat.shots);
   await page.waitForTimeout(1000);assert.equal(await page.evaluate(()=>racingState.combat.shots),released,'Resume cannot latch fire');
   await page.keyboard.down('e');await page.keyboard.down('f');
   await page.waitForFunction(()=>racingState.combat.releaseRequired,{},{timeout:10000});
   await page.waitForFunction(()=>racingState.combat.charge>=30,{},{timeout:10000});
   assert.equal(await page.evaluate(()=>racingState.combat.shield),false,'Empty shield stays down while E is held');
   await page.waitForFunction(()=>document.getElementById('shield-state').textContent==='RELEASE E TO RESET');
   await page.screenshot({path:path.join(out,backend+'-shield-depleted.png')});
   const depleted=await page.evaluate(()=>racingState.combat);
   await page.keyboard.up('e');await page.keyboard.up('f');
   await page.waitForFunction(()=>!racingState.combat.releaseRequired);
   await page.keyboard.down('e');await page.waitForFunction(()=>racingState.combat.shield);
   await page.keyboard.up('e');
   report.backends.push({backend,paused,depleted,checks:['narrow HUD geometry','shield energy consumption','pause freezes combat','resume clears held fire and shield','depletion lockout','energy regeneration','release and reactivation']});
   await page.close();
  }
  assert.deepEqual(report.errors,[]);
 }finally{
  await fs.writeFile(path.join(out,'results.json'),JSON.stringify(report,null,2));await browser.close();
 }
 console.log(JSON.stringify(report,null,2));
})().catch(e=>{console.error(e);process.exitCode=1;});
