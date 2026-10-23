// Run serially against one frozen generated consumer build.
const assert=require('node:assert/strict');
const fs=require('node:fs/promises');
const path=require('node:path');
const {chromium}=require('playwright');
const {PNG}=require('pngjs');
const base=process.env.DEMO_URL||'http://127.0.0.1:8098';
const out=path.resolve(process.env.EVIDENCE_DIR||'build/evidence/shield-effect');
const chassis=process.env.SHIELD_CHASSIS?.split(',').map(Number);
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:process.env.BROWSER_EXECUTABLE||'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 const report={browser:await browser.version(),backends:[]};
 try {
  for(const backend of ['webgpu','webgl2']) {
   const context=await browser.newContext({viewport:{width:1440,height:900}});
   const page=await context.newPage(),errors=[];
   page.on('pageerror',e=>errors.push(String(e)));
   page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
   await page.goto(`${base}/?renderer=${backend}`);
   await page.waitForFunction(()=>window.racingReady&&window.racingShieldVisual,null,{timeout:120000});
   assert.equal(await page.evaluate(()=>racingBackend),backend);
   const record={backend,build:await page.evaluate(async()=>await(await fetch('build-info.json')).json()),vehicles:[],errors};
   for(const [id,button,name] of [[0,'car','vektor'],[2,'bastion','bastion'],[1,'hover','skimmer'],[3,'vector','vector']]) {
    if(chassis&&!chassis.includes(id))continue;
    await page.locator('#'+button).click();
    await page.waitForFunction(id=>racingState.chassis===id&&racingState.phase===0,id);
    assert(!await page.evaluate(()=>racingShieldVisual.visible));
    await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);
    const before=PNG.sync.read(await page.screenshot());
    await page.keyboard.down('e');
    await page.waitForFunction(()=>racingState.combat.shield&&racingShieldVisual.visibility===1);
    const active=await page.screenshot({path:path.join(out,`${backend}-${name}-active.png`)});
    const after=PNG.sync.read(active);let brighter=0;
    // Real world pixels around the player; excludes HUD and the other grid rows.
    for(let y=480;y<800;y++)for(let x=570;x<870;x++) {
     const i=(y*after.width+x)*4;
     if(after.data[i+1]+after.data[i+2]>before.data[i+1]+before.data[i+2]+35)brighter++;
    }
    assert(brighter>150,`${backend}/${name}: shield must change actual car-region pixels (${brighter})`);
    await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
    const paused=await page.evaluate(()=>({combat:racingState.combat,visual:racingShieldVisual}));
    await page.waitForTimeout(250);
    assert.deepEqual(await page.evaluate(()=>({combat:racingState.combat,visual:racingShieldVisual})),paused);
    await page.keyboard.up('e');await page.locator('#resume').click();
    await page.waitForFunction(()=>!racingShieldVisual.visible);
    await page.keyboard.down('e');await page.waitForFunction(()=>racingState.combat.shield);
    await page.waitForFunction(()=>racingShieldVisual.collapse>0,null,{timeout:10000});
    const depleted=await page.evaluate(()=>({combat:racingState.combat,visual:racingShieldVisual}));
    assert(!depleted.combat.shield&&depleted.combat.releaseRequired&&depleted.visual.visible);
    await page.screenshot({path:path.join(out,`${backend}-${name}-collapse.png`)});
    await page.waitForFunction(()=>!racingShieldVisual.visible);
    await page.waitForTimeout(400);assert(!await page.evaluate(()=>racingShieldVisual.visible));
    await page.keyboard.up('e');
    await page.waitForFunction(()=>racingState.combat.charge>=30);
    await page.keyboard.down('e');await page.waitForFunction(()=>racingShieldVisual.visibility===1);
    await page.keyboard.down('w');await page.waitForFunction(()=>racingState.speed>15);
    await page.screenshot({path:path.join(out,`${backend}-${name}-moving.png`)});
    await page.keyboard.up('w');await page.keyboard.up('e');await page.keyboard.press('r');
    await page.waitForFunction(()=>racingState.recoveries>0&&!racingShieldVisual.visible);
    await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
    await page.locator('#pause-panel .garage-button').click();await page.waitForFunction(()=>racingState.phase===0);
    assert(!await page.evaluate(()=>racingShieldVisual.visible));
    record.vehicles.push({name,brighter,depleted,checks:['visible world pixels','pause freezes effect','release fades','depletion collapse','held depletion stays off','moving field','recovery reset','garage reset']});
   }
   await page.setViewportSize({width:390,height:844});
   await page.locator('#car').click();await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);
   await page.keyboard.down('e');await page.waitForFunction(()=>racingShieldVisual.visibility===1);
   await page.screenshot({path:path.join(out,`${backend}-390px.png`)});await page.keyboard.up('e');
   assert.deepEqual(errors,[]);report.backends.push(record);
   await fs.writeFile(path.join(out,'results.json'),JSON.stringify(report,null,2));
   await context.close();console.log(`${backend}: ${record.vehicles.length} chassis passed`);
  }
 }catch(e){report.failure=String(e.stack||e);throw e;}
 finally {await fs.writeFile(path.join(out,'results.json'),JSON.stringify(report,null,2));await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
