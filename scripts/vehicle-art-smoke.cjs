// Run on the generated consumer site, serially across renderer backends.
const assert=require('node:assert/strict');
const fs=require('node:fs/promises');
const path=require('node:path');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8093';
const out=path.resolve(process.env.EVIDENCE_DIR||'build/evidence/vehicle-art');
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:process.env.BROWSER_EXECUTABLE||'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 const report={browser:await browser.version(),backends:{}};
 try{
  for(const backend of ['webgpu','webgl2']){
   const context=await browser.newContext({viewport:{width:1440,height:900}});
   const page=await context.newPage(),errors=[];
   page.on('pageerror',e=>errors.push(String(e)));
   page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
   await page.route('**/bastion.glb',r=>r.fulfill({status:200,contentType:'model/gltf-binary',body:Buffer.from('bad asset')}),{times:1});
   await page.goto(`${base}/?renderer=${backend}`);
   await page.waitForFunction(()=>window.racingCarError,null,{timeout:90000});
   assert(!await page.evaluate(()=>!!window.racingReady));
   await page.getByRole('button',{name:'RETRY VEHICLE DOWNLOAD',exact:true}).click();
   await page.waitForFunction(()=>window.racingReady&&window.racingState,null,{timeout:90000});
   assert.equal(await page.evaluate(()=>racingBackend),backend);
   const assets=await page.evaluate(()=>racingVehicleAssets);
   for(const [id,name] of [[1,'skimmer'],[2,'bastion'],[3,'vector']]){
    const record=JSON.parse(await fs.readFile(`assets/runtime/models/${name}/${name}.asset.json`,'utf8'));
    assert.equal(assets[id].sha256,record.sha256);
   }
   assert.deepEqual(await page.evaluate(()=>[...new Set(racingState.standings.filter(r=>r.id).map(r=>r.chassis))].sort()),[0,1,2,3]);
   const vehicles=[];
   for(const [id,button,name] of [[0,'car','Vektor'],[2,'bastion','Bastion'],[1,'hover','Skimmer'],[3,'vector','Vector']]){
    await page.locator('#'+button).click();
    await page.waitForFunction(id=>racingState.chassis===id&&racingState.phase===0,id);
    await page.waitForTimeout(180);
    assert.equal(await page.locator('#'+button).getAttribute('aria-pressed'),'true');
    await page.screenshot({path:path.join(out,`${backend}-${name.toLowerCase()}-garage.png`)});
    await page.locator('#start').click();
    await page.waitForFunction(()=>racingState.phase===2);
    await page.keyboard.down('KeyW');await page.waitForTimeout(1800);
    const moving=await page.evaluate(()=>({speed:racingState.speed,wheels:racingWheels}));
    assert(moving.speed>15,`${name} must accelerate`);
    if(id===2){
     assert.deepEqual(moving.wheels.map(w=>w.name),['wheel_front_left','wheel_front_right','wheel_rear_left','wheel_rear_right']);
     assert(moving.wheels.every(w=>Math.abs(w.spin)>.1));
    }
    await page.keyboard.down('KeyD');await page.waitForTimeout(450);await page.keyboard.up('KeyD');
    if(id===2){const wheels=await page.evaluate(()=>racingWheels);assert(wheels.slice(0,2).every(w=>Math.abs(w.steer)>.02));assert(wheels.slice(2).every(w=>Math.abs(w.steer)<.001));}
    await page.keyboard.up('KeyW');
    await page.keyboard.down('KeyF');await page.keyboard.down('KeyE');await page.waitForTimeout(200);
    const combat=await page.evaluate(()=>racingState.combat);assert(combat.shots>0&&combat.shield);
    await page.keyboard.up('KeyF');await page.keyboard.up('KeyE');
    await page.screenshot({path:path.join(out,`${backend}-${name.toLowerCase()}-race.png`)});
    const recoveries=await page.evaluate(()=>racingState.recoveries);
    await page.keyboard.press('KeyR');await page.waitForFunction(r=>racingState.recoveries===r+1,recoveries);
    await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
    const elapsed=await page.evaluate(()=>racingState.elapsed);await page.waitForTimeout(160);assert.equal(await page.evaluate(()=>racingState.elapsed),elapsed);
    await page.locator('#pause-panel .garage-button').click();await page.waitForFunction(()=>racingState.phase===0);
    assert.equal(await page.evaluate(()=>racingState.chassis),id);
    vehicles.push({name,speed:moving.speed,combat,checks:['selection','acceleration','steering','pulse and shield','recovery','pause','return to same chassis']});
   }
   await page.reload();await page.waitForFunction(()=>window.racingReady&&racingState.chassis===3,null,{timeout:90000});
   assert.equal(await page.locator('#vector').getAttribute('aria-pressed'),'true');
   await page.setViewportSize({width:390,height:844});await page.waitForTimeout(150);
   await page.locator('#start').scrollIntoViewIfNeeded();
   assert(await page.locator('#start').isVisible());
   assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
   await page.screenshot({path:path.join(out,`${backend}-narrow.png`)});
   assert.deepEqual(errors,[]);
   report.backends[backend]={assets,vehicles,errors,build:await page.evaluate(async()=>await(await fetch('build-info.json')).json()),checks:['hash mismatch rejected and retried','AI field uses four chassis','persisted chassis selection','390px garage and fixed start action']};
   await fs.writeFile(path.join(out,'results.json'),JSON.stringify(report,null,2));
   await context.close();console.log(backend+' vehicle art passed');
  }
 }catch(error){report.failure=String(error.stack||error);throw error;}
 finally{await fs.writeFile(path.join(out,'results.json'),JSON.stringify(report,null,2));await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
