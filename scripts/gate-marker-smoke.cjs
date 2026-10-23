// Actual chase-camera acceptance against a frozen site, serial renderer runs.
const assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8114';
const out=path.resolve(process.env.EVIDENCE_DIR||'build/evidence/gate-marker');
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:process.env.BROWSER_EXECUTABLE||'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 const track=process.env.TRACK||'kairox';
 const report={browser:await browser.version(),track,backends:{}};
 delete report.failure;
 try{for(const backend of (process.env.RENDERERS||'webgpu,webgl2').split(',')){
  const context=await browser.newContext({viewport:{width:1440,height:900}}),page=await context.newPage(),errors=[];
  page.on('pageerror',e=>errors.push(String(e)));page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
  await page.route('**/next-gate-beacon.glb',r=>r.fulfill({status:200,body:Buffer.from('corrupt beacon')}),{times:1});
  await page.goto(`${base}/?track=${track}&renderer=${backend}`);
  await page.waitForFunction(()=>window.racingCarError?.includes('out of date'),null,{timeout:180000});
  assert(!await page.evaluate(()=>!!window.racingReady));
  await page.evaluate(()=>Module._race_action(1));await page.waitForTimeout(100);
  assert.equal(await page.evaluate(()=>racingState.phase),0);
  await page.getByRole('button',{name:'RETRY BEACON DOWNLOAD',exact:true}).click();
  await page.waitForFunction(()=>window.racingReady&&window.racingGateMarker,null,{timeout:180000});
  assert.equal(await page.evaluate(()=>racingBackend),backend);
  assert(!await page.evaluate(()=>racingGateMarker.visible));
  const asset=await page.evaluate(()=>racingGateAssets[0]);
  const record=JSON.parse(await fs.readFile('assets/runtime/models/navigation/next-gate-beacon/next-gate-beacon.asset.json','utf8'));
  assert.equal(asset.sha256,record.sha256);
  await page.locator('#hover').click();await page.locator('#start').click();
  await page.waitForFunction(()=>racingState.phase===2&&racingGateMarker.visible);
  await page.evaluate(()=>{
   window.beaconSeen=[];window.beaconFailures=[];window.beaconApproaches={};
   window.beaconPilot=setInterval(()=>{
    const s=racingState,m=racingGateMarker;if(s.phase!==2||s.paused)return;
    const gate=racingTrack[s.gate*16];
    if(m.gate!==s.gate||!m.visible||m.mountHeight<7||Math.hypot(m.x-gate[0],m.z-gate[1])>.01)beaconFailures.push({s,m});
    if(!beaconSeen.includes(s.gate))beaconSeen.push(s.gate);
    const distance=Math.hypot(s.x-gate[0],s.z-gate[1]);
    if(distance>18&&distance<28)beaconApproaches[s.gate]=true;
    let best=Infinity,index=0;
    for(let i=0;i<racingTrack.length;i++){const p=racingTrack[i],d=(p[0]-s.x)**2+(p[1]-s.z)**2;if(d<best){best=d;index=i;}}
    const t=racingTrack[(index+6)%racingTrack.length];let a=Math.atan2(t[0]-s.x,t[1]-s.z)-s.heading;
    a=Math.atan2(Math.sin(a),Math.cos(a));
    Module._race_input(101,900,Math.max(-1,Math.min(1,-a*2)));
    Module._race_input(100,901,s.speed<65?1:.1);Module._race_input(102,902,s.speed>76?.5:0);
   },16);
  });
  for(const gate of [1,2,3,4,5,6,7,8,9,10,11,0]) {
   await page.waitForFunction(g=>beaconApproaches[g],gate,{timeout:90000});
   await page.screenshot({path:path.join(out,`${backend}-approach-${gate}.png`)});
  }
  await page.waitForFunction(()=>racingState.lap>=1,null,{timeout:90000});
  await page.evaluate(()=>{clearInterval(beaconPilot);Module._race_cancel();Module._race_action(5);});
  await page.waitForFunction(()=>racingState.paused);
  const lap=await page.evaluate(()=>({state:racingState,marker:racingGateMarker,seen:beaconSeen,failures:beaconFailures}));
  assert.equal(lap.state.recoveries,0);assert(lap.state.valid&&lap.state.best>0);
  assert.equal(lap.seen.length,12);assert.deepEqual(lap.failures,[]);
  await page.waitForTimeout(250);assert.deepEqual(await page.evaluate(()=>racingGateMarker),lap.marker);
  await page.evaluate(()=>Module._race_action(4));await page.waitForTimeout(150);
  assert.equal(await page.evaluate(()=>racingGateMarker.gate),lap.marker.gate);
  await page.evaluate(()=>Module._race_action(2));await page.waitForFunction(()=>racingState.phase===0);
  assert(!await page.evaluate(()=>racingGateMarker.visible));
  await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);
  await page.evaluate(()=>{
   window.cameraPilot=setInterval(()=>{
    const s=racingState;let best=Infinity,index=0;
    for(let i=0;i<racingTrack.length;i++){const p=racingTrack[i],d=(p[0]-s.x)**2+(p[1]-s.z)**2;if(d<best){best=d;index=i;}}
    const t=racingTrack[(index+4)%racingTrack.length];let a=Math.atan2(t[0]-s.x,t[1]-s.z)-s.heading;a=Math.atan2(Math.sin(a),Math.cos(a));
    Module._race_input(101,900,Math.max(-1,Math.min(1,-a*2)));
    Module._race_input(100,901,s.speed<40?1:0);Module._race_input(102,902,s.speed>45?.5:0);
   },16);
  });
  await page.waitForFunction(()=>{const s=racingState,p=racingTrack[16];return s.elapsed>2&&s.speed>10&&racingWheels.some(w=>Math.abs(w.omega)>1)&&Math.hypot(s.x-p[0],s.z-p[1])<25;},null,{timeout:30000});
  await page.screenshot({path:path.join(out,`${backend}-car-driving.png`)});
  const wheels=await page.evaluate(()=>racingWheels);assert(wheels.some(w=>Math.abs(w.omega)>1));
  await page.evaluate(()=>{clearInterval(cameraPilot);Module._race_cancel();});
  await page.setViewportSize({width:900,height:600});await page.waitForTimeout(300);
  await page.screenshot({path:path.join(out,`${backend}-narrow-driving.png`)});
  assert.deepEqual(errors,[]);
  report.backends[backend]={asset,lap:lap.state.best,seen:lap.seen,markerFailures:lap.failures,wheels: wheels.length,errors};
  await fs.writeFile(path.join(out,'report.json'),JSON.stringify(report,null,2));
  await context.close();console.log(`${backend}: beacon retry, all gates, chase views and wheels passed`);
 }}catch(e){report.failure=String(e.stack||e);throw e;}
 finally{await fs.writeFile(path.join(out,'report.json'),JSON.stringify(report,null,2));await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
