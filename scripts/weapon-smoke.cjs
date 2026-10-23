// Verify one frozen consumer site serially on both renderers. Uses explicit career fixtures.
const assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8102';
const out=path.resolve(process.env.EVIDENCE_DIR||'build/evidence/weapon');
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:process.env.BROWSER_EXECUTABLE||'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 const report={browser:await browser.version(),backends:[]};
 try{for(const backend of ['webgpu','webgl2']){
  const origin=new URL(base).origin;
  const context=await browser.newContext({viewport:{width:1440,height:900},storageState:{cookies:[],origins:[{origin,localStorage:[{name:'neon-ascension.career.v1',value:'NA2 10000 5 0 0 0 0 0 0 0 0'}]}]}});
  const page=await context.newPage(),errors=[];
  page.on('pageerror',e=>errors.push(String(e)));page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
  await page.route('**/weapons/pulse-cannon/pulse-cannon.glb',r=>r.fulfill({status:200,contentType:'model/gltf-binary',body:Buffer.from('corrupt weapon')}),{times:1});
  await page.goto(`${base}/?renderer=${backend}&weaponReview=1`);
  await page.waitForFunction(()=>window.racingCarError?.includes('pulse-cannon'),null,{timeout:120000});
  assert(!await page.evaluate(()=>!!window.racingReady));
  await page.locator('#start').click();
  await page.waitForFunction(()=>window.racingReady&&window.racingWeaponVisual?.loaded,null,{timeout:120000});
  assert.equal(await page.evaluate(()=>racingBackend),backend);
  const asset=await page.evaluate(()=>racingWeaponAssets[0]);
  const contract=JSON.parse(await fs.readFile('assets/runtime/models/weapons/pulse-cannon/pulse-cannon.asset.json','utf8'));
  assert.equal(asset.sha256,contract.sha256);
  const result={backend,asset,errors,vehicles:[],build:await page.evaluate(async()=>await(await fetch('build-info.json')).json())};
  for(const [id,button,name] of [[0,'car','vektor'],[2,'bastion','bastion'],[1,'hover','skimmer'],[3,'vector','vector']]){
   await page.locator('#'+button).click();await page.waitForFunction(id=>racingState.chassis===id,id);
   if(id===0||id===1){
    assert.equal(await page.evaluate(()=>racingWeaponVisual.rank),0);
    await page.screenshot({path:path.join(out,`${backend}-${name}-stock.png`)});
    for(let rank=1;rank<=3;rank++){
     await page.locator('#buy-1').click();await page.waitForFunction(rank=>racingWeaponVisual.rank===rank,rank);
     await page.screenshot({path:path.join(out,`${backend}-${name}-mk${rank+1}.png`)});
    }
   }
   assert.equal(await page.evaluate(()=>racingWeaponVisual.rank),3);
   await page.screenshot({path:path.join(out,`${backend}-${name}-mounted.png`)});
   await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);
   await page.keyboard.down('f');await page.waitForFunction(()=>racingState.combat.shots>=2&&racingWeaponVisual.recoil>.025);
   await page.screenshot({path:path.join(out,`${backend}-${name}-fire.png`)});
   const combat=await page.evaluate(()=>racingState.combat);assert(combat.cooldown<=.501);
   await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
   const frozen=await page.evaluate(()=>racingWeaponVisual);await page.waitForTimeout(250);
   assert.deepEqual(await page.evaluate(()=>racingWeaponVisual),frozen);
   await page.keyboard.up('f');await page.locator('#resume').click();
   await page.waitForFunction(()=>racingWeaponVisual.recoil===0);
   // Switch back to the normal chase camera and verify equipment in motion.
   await page.evaluate(()=>{const url=new URL(location.href);url.searchParams.delete('weaponReview');history.replaceState(null,'',url);});
   await page.keyboard.down('w');await page.waitForFunction(()=>racingState.speed>15);
   await page.screenshot({path:path.join(out,`${backend}-${name}-chase.png`)});
   await page.keyboard.up('w');await page.keyboard.press('r');
   await page.waitForFunction(()=>racingState.recoveries>0&&racingWeaponVisual.recoil===0);
   await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
   await page.locator('#pause-panel .garage-button').click();await page.waitForFunction(()=>racingState.phase===0);
   assert.equal(await page.evaluate(()=>racingWeaponVisual.rank),3);
   await page.evaluate(()=>{const url=new URL(location.href);url.searchParams.set('weaponReview','1');history.replaceState(null,'',url);});
   result.vehicles.push({name,combat,checks:['mounts','class upgrade ranks','firing recoil','pause','chase driving','recovery','garage return']});
  }
  await page.reload();await page.waitForFunction(()=>window.racingReady&&racingWeaponVisual.rank===3,null,{timeout:120000});
  await page.setViewportSize({width:390,height:844});await page.waitForTimeout(200);
  await page.locator('#buy-1').scrollIntoViewIfNeeded();assert(await page.locator('#buy-1').isVisible());
  assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
  await page.screenshot({path:path.join(out,`${backend}-narrow.png`)});
  assert.deepEqual(errors,[]);report.backends.push(result);await context.close();
 }}finally{await fs.writeFile(path.join(out,'report.json'),JSON.stringify(report,null,2));await browser.close();}
 console.log(JSON.stringify(report,null,2));
})().catch(e=>{console.error(e);process.exit(1);});
