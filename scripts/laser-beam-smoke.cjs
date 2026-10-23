// Serial validation of the actual generated consumer game, no fixture weapon.
const assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path');
const {chromium}=require('playwright'),{PNG}=require('pngjs');
const base=process.env.DEMO_URL||'http://127.0.0.1:8110',out=path.resolve('build/evidence/laser-beam');
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:process.env.BROWSER_EXECUTABLE||'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 const report={browser:await browser.version(),backends:[]};
 try{for(const backend of ['webgpu','webgl2']) {
  const context=await browser.newContext({viewport:{width:1440,height:900}}),page=await context.newPage(),errors=[];
  page.setDefaultTimeout(120000);page.on('pageerror',e=>errors.push(String(e)));page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
  await page.goto(`${base}/?renderer=${backend}`);
  await page.waitForFunction(()=>window.racingCarError||window.racingReady);
  assert(!await page.evaluate(()=>window.racingCarError));assert.equal(await page.evaluate(()=>racingBackend),backend);
  const record={backend,build:await page.evaluate(async()=>await(await fetch('build-info.json')).json()),vehicles:[],errors};
  assert.equal(record.build.developmentEngine,false);assert.equal(record.build.engineDirty,false);
  for(const name of ['car','bastion','hover','vector']) {
   await page.locator('#'+name).click();await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);
   assert(!await page.evaluate(()=>racingLaserVisual.visible));
   const before=PNG.sync.read(await page.screenshot());
   // Observe a genuine keyboard-fired pulse and immediately pause through the
   // same public command as Escape, so slow software rendering cannot miss it.
   await page.evaluate(()=>{window.laserCaptured=false;function capture(){if(racingLaserVisual.visible){Module._race_action(6);window.laserCaptured=true;}else requestAnimationFrame(capture);}requestAnimationFrame(capture);});
   await page.keyboard.down('f');await page.waitForFunction(()=>window.laserCaptured&&racingState.paused);await page.keyboard.up('f');
   const fired=await page.evaluate(()=>({visual:racingLaserVisual,combat:racingState.combat}));
   assert(fired.visual.visible&&fired.visual.length>0&&fired.visual.length<=42.01);
   await page.waitForTimeout(250);assert.deepEqual(await page.evaluate(()=>({visual:racingLaserVisual,combat:racingState.combat})),fired);
   const shot=PNG.sync.read(await page.screenshot({path:path.join(out,`${backend}-${name}-laser.png`),style:'#pause-panel { visibility:hidden !important; }'}));
   let cyan=0;
   for(let y=260;y<650;y++)for(let x=400;x<1040;x++){
    const i=(y*shot.width+x)*4;
    if(shot.data[i+1]>110&&shot.data[i+2]>140&&shot.data[i+2]>shot.data[i]+25&&shot.data[i+1]+shot.data[i+2]>before.data[i+1]+before.data[i+2]+55)cyan++;
   }
   assert(cyan>20,`${backend}/${name}: visible beam pixels (${cyan})`);
   await page.locator('#resume').click();await page.waitForFunction(()=>!racingLaserVisual.visible);
   const shots=await page.evaluate(()=>racingState.combat.shots);
   await page.waitForTimeout(300);assert.equal(await page.evaluate(()=>racingState.combat.shots),shots);
   await page.keyboard.down('f');await page.waitForFunction(n=>racingState.combat.shots>=n+2,shots);await page.keyboard.up('f');
   // Sample consecutive rendered frames of the SAME pulse while driving. The
   // old frozen world segment fails even though its stationary screenshot passes.
   await page.keyboard.down('w');await page.waitForFunction(()=>racingState.speed>20);
   await page.evaluate(()=>{
    window.laserMotion={samples:[],done:false};
    function sample(){
     const m=window.laserMotion,s=window.racingState,v=window.racingLaserVisual;
     if(v.visible){
      const last=m.samples.at(-1),now={shot:s.combat.shots,origin:[...v.origin],x:s.x,z:s.z,length:v.length};
      m.samples.push(now);
      if(last&&last.shot===now.shot&&Math.hypot(now.x-last.x,now.z-last.z)>.04){
       m.pair=[last,now];m.done=true;Module._race_action(6);return;
      }
     }
     requestAnimationFrame(sample);
    }requestAnimationFrame(sample);
   });
   await page.keyboard.down('f');await page.waitForFunction(()=>laserMotion.done&&racingState.paused);
   await page.keyboard.up('f');await page.keyboard.up('w');
   const motion=await page.evaluate(()=>laserMotion.pair),[a,b]=motion;
   const beamTravel=Math.hypot(...b.origin.map((v,i)=>v-a.origin[i]));
   assert(beamTravel>.02,`${backend}/${name}: live beam must move with the car (${beamTravel})`);
   assert(Math.abs(b.length-a.length)<.01,'Following keeps the original pulse length');
   await page.screenshot({path:path.join(out,`${backend}-${name}-moving-laser.png`),style:'#pause-panel { visibility:hidden !important; }'});
   await page.locator('#resume').click();
   await page.keyboard.press('r');await page.waitForFunction(()=>racingState.recoveries>0&&!racingLaserVisual.visible);
   await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
   await page.locator('#pause-panel .garage-button').click();await page.waitForFunction(()=>racingState.phase===0&&!racingLaserVisual.visible);
   record.vehicles.push({name,cyan,fired,motion,beamTravel,checks:['real F fire','rendered beam pixels','simulation pause','release expiry','held repeat','moving pulse follows car','recovery','garage reset']});
   console.log(backend,name,'passed',cyan);
  }
  assert.deepEqual(errors,[]);report.backends.push(record);await fs.writeFile(path.join(out,'report.json'),JSON.stringify(report,null,2));await context.close();
 }}catch(e){report.failure=String(e.stack||e);throw e;}
 finally{await fs.writeFile(path.join(out,'report.json'),JSON.stringify(report,null,2));await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
