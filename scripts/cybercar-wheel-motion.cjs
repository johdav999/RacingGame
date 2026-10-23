// Real keyboard driving plus inspection-only body drops; wheel poses stay physics-owned.
const {chromium}=require('playwright');
const assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path');
const bastion=process.env.CHASSIS==='bastion';
const out=path.resolve(process.env.EVIDENCE_DIR||`build/evidence/${bastion?'bastion':'cybercar'}-wheel-repair/motion`);
const base=process.env.DEMO_URL||'http://127.0.0.1:8112';
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 const report={};let activePage;
 try{for(const backend of ['webgpu','webgl2']){
  const page=await browser.newPage({viewport:{width:1440,height:900}}),errors=[];
  activePage=page;
  report[backend]={errors};
  page.on('pageerror',e=>{errors.push(String(e));console.error(String(e))});page.on('console',m=>{if(m.type()==='error'){errors.push(m.text());console.error(m.text())}});
  await page.goto(`${base}/?track=desert&renderer=${backend}&wheelReview=1&mode=trial`);
  await page.waitForFunction(()=>window.racingReady,null,{timeout:180000});
  assert.equal(await page.evaluate(()=>racingBackend),backend);
  const expected=JSON.parse(await fs.readFile(bastion?'assets/runtime/models/bastion/bastion.asset.json':'assets/runtime/models/cybercar-clean-detail/cybercar.asset.json'));
  assert.equal(await page.evaluate(b=>b?racingVehicleAssets[2].sha256:racingCarAsset.sha256,bastion),expected.sha256);
  if(bastion){await page.locator('#bastion').click();await page.waitForFunction(()=>racingState.chassis===2)}
  await page.evaluate(()=>{
   window.wheelMotionSamples=[];window.recordWheelMotion=true;
   const collect=()=>{if(window.recordWheelMotion&&window.racingWheels?.length===4)window.wheelMotionSamples.push({time:performance.now(),wheels:structuredClone(racingWheels)});requestAnimationFrame(collect)};collect();
  });
  await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);
  const shots=[];
  const capture=async name=>{const file=`${backend}-${name}.png`;await page.screenshot({path:path.join(out,file),style:'header,main,footer,.vignette{visibility:hidden!important}'});shots.push({file,wheels:await page.evaluate(()=>racingWheels)});};
  for(const [side,key] of [[1,'a'],[2,'d']]){
   await page.evaluate(side=>window.racingWheelReviewSide=side,side);
   const recoveries=await page.evaluate(()=>racingState.recoveries);
   await page.keyboard.press('r');
   await page.waitForFunction(n=>racingState.recoveries>n,recoveries,{timeout:10000});
   await page.keyboard.down('w');await page.keyboard.down(key);
   // Coast slowly for readable close-ups without reaching the roadside barrier.
   await page.waitForFunction(()=>racingState.speed>8);await page.keyboard.up('w');
   for(let frame=0;frame<2;frame++){await page.waitForTimeout(70);await capture(`side-${side}-frame-${frame}`);}
   // Exercise full bump/droop with the same physical drop as the native test.
   // Raising the rigid body never overrides wheel spin, steering or local poses.
   await page.evaluate(()=>window.racingWheelReviewDrop=true);
   await page.waitForFunction(()=>window.racingWheelReviewDrop===false);
   for(let frame=0;frame<12;frame++){await page.waitForTimeout(40);await capture(`side-${side}-drop-${frame}`);}
   for(let frame=2;frame<8;frame++){await page.waitForTimeout(70);await capture(`side-${side}-frame-${frame}`);}
   await page.keyboard.up(key);await page.keyboard.up('w');
  }
  await page.evaluate(()=>window.recordWheelMotion=false);
  const samples=await page.evaluate(()=>wheelMotionSamples);assert(samples.length>20);
  report[backend]={asset:expected.sha256,shots,samples,errors};
  const ranges=[0,1,2,3].map(i=>{
   const states=samples.map(s=>s.wheels[i]);
   const minimum=Math.min(...states.map(w=>w.suspension)),maximum=Math.max(...states.map(w=>w.suspension));
   assert(minimum>=expected.suspension.minimum-.01001&&maximum<=expected.suspension.maximum+.001,`Suspension stays within authored envelope including bump-stop tolerance: wheel ${i} ${minimum}..${maximum}`);
   assert(minimum<expected.suspension.minimum+.02&&maximum>expected.suspension.maximum-.001,'Runtime captures full bump and droop');
   assert(Math.max(...states.map(w=>Math.abs(w.omega)))>1,'Wheel actually rotates');
   if(i<2){assert(Math.min(...states.map(w=>w.steer))<-.15);assert(Math.max(...states.map(w=>w.steer))>.15)}
   else assert(states.every(w=>Math.abs(w.steer)<1e-6));
   return {name:states[0].name,minimum,maximum};
  });
  assert.deepEqual(errors,[]);
  report[backend]={asset:expected.sha256,ranges,shots,samples,errors};
  await page.close();console.log(backend,JSON.stringify(ranges));
 }}catch(error){
  report.failure={message:String(error),state:await activePage?.evaluate(()=>({race:window.racingState,wheels:window.racingWheels,drop:window.racingWheelReviewDrop,hidden:document.hidden})).catch(()=>null)};
  throw error;
 }finally{await fs.writeFile(path.join(out,'motion.json'),JSON.stringify(report,null,2));await browser.close();}
 const frames=Object.values(report).flatMap(r=>r.shots.map(s=>s.file));
 await fs.writeFile(path.join(out,'index.html'),`<!doctype html><title>Runtime wheel motion</title><style>body{background:#111;color:white;font:18px system-ui}img{width:95vw}button{margin:12px}</style><h1>Actual WebGPU / WebGL2 wheel motion</h1><p>Real keyboard steering and recovery drops. Physics controls all wheel transforms.</p><button onclick="play=!play">Play / pause</button><span id="name"></span><br><img id="image"><script>let play=true,i=0;const frames=${JSON.stringify(frames)};setInterval(()=>{if(play){document.querySelector('#image').src=frames[i];document.querySelector('#name').textContent=frames[i];i=(i+1)%frames.length}},180)</script>`);
})().catch(e=>{console.error(e);process.exitCode=1});
