// Capture closer car approaches without overriding the chase rig or hiding UI.
const {chromium}=require('playwright');
const assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path');
(async()=>{
 const out=path.resolve(process.env.EVIDENCE_DIR||'build/evidence/gate-marker'),report={};
 const track=process.env.TRACK||'kairox';await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 try{for(const backend of (process.env.RENDERERS||'webgpu,webgl2').split(',')){
  const page=await browser.newPage({viewport:{width:1440,height:900}}),errors=[];
  page.on('pageerror',e=>errors.push(String(e)));
  await page.goto(`${process.env.DEMO_URL||'http://127.0.0.1:8114'}/?track=${track}&renderer=${backend}`);
  await page.waitForFunction(()=>window.racingReady,null,{timeout:180000});
  assert.equal(await page.evaluate(()=>racingBackend),backend);
  await page.locator('#car').click();await page.locator('#start').click();
  await page.waitForFunction(()=>racingState.phase===2);
  await page.evaluate(()=>{
   window.cameraPilot=setInterval(()=>{
    const s=racingState;let best=Infinity,index=0;
    for(let i=0;i<racingTrack.length;i++){const p=racingTrack[i],d=(p[0]-s.x)**2+(p[1]-s.z)**2;if(d<best){best=d;index=i;}}
    const t=racingTrack[(index+4)%racingTrack.length];let a=Math.atan2(t[0]-s.x,t[1]-s.z)-s.heading;a=Math.atan2(Math.sin(a),Math.cos(a));
    Module._race_input(101,900,Math.max(-1,Math.min(1,-a*2)));
    Module._race_input(100,901,s.speed<40?1:0);Module._race_input(102,902,s.speed>45?.5:0);
   },16);
  });
  for(const distance of [50,30,18]) {
   await page.waitForFunction(d=>{const s=racingState,p=racingTrack[16];return s.gate===1&&s.elapsed>1&&s.speed>10&&Math.hypot(s.x-p[0],s.z-p[1])<d;},distance,{timeout:60000});
   await page.screenshot({path:path.join(out,`${backend}-approach-${distance}m.png`)});
   if(distance===50) {
    await page.setViewportSize({width:900,height:600});await page.waitForTimeout(100);
    await page.screenshot({path:path.join(out,`${backend}-narrow-approach.png`)});
    await page.setViewportSize({width:1440,height:900});
   }
  }
  await page.screenshot({path:path.join(out,`${backend}-car-beacon.png`)});
  report[backend]=await page.evaluate(()=>({state:racingState,marker:racingGateMarker,asset:racingGateAssets[0],wheels:racingWheels}));
  assert(report[backend].marker.visible);assert(report[backend].wheels.some(w=>Math.abs(w.omega)>1));
  assert(report[backend].marker.mountHeight>(track==='desert'?10:7));
  await page.setViewportSize({width:900,height:600});await page.waitForTimeout(200);
  await page.screenshot({path:path.join(out,`${backend}-narrow-beacon.png`)});
  await page.evaluate(()=>{clearInterval(cameraPilot);Module._race_cancel();});
  assert.deepEqual(errors,[]);await page.close();console.log(`${backend}: car and narrow beacon views captured`);
 }}finally{await browser.close();await fs.writeFile(path.join(out,'camera-report.json'),JSON.stringify(report,null,2));}
})().catch(e=>{console.error(e);process.exitCode=1;});
