// Focused real-window visibility, failed-download retry and audible drift checks.
const assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8113',out=path.resolve('build/evidence/race-audio');
(async()=>{
 const browser=await chromium.launch({headless:false,executablePath:'C:/Program Files/Google/Chrome/Application/chrome.exe',ignoreDefaultArgs:['--disable-backgrounding-occluded-windows'],args:['--force-high-performance-gpu','--window-position=-16000,-16000']});
 const report={checks:[]};
 try {
  const context=await browser.newContext({viewport:{width:1280,height:800}}),page=await context.newPage();page.setDefaultTimeout(120000);
  await page.route('**/audio/racing/shield-impact.wav',route=>route.fulfill({status:503,body:'Test unavailable'}));
  await page.goto(base+'/?track=desert&renderer=webgpu');
  await page.waitForFunction(()=>window.racingAudioError&&window.racingReady);
  assert.equal(await page.evaluate(()=>racingAudioTelemetry.ready),false);
  await page.unroute('**/audio/racing/shield-impact.wav');
  await page.locator('#audio-settings summary').click();await page.locator('#audio-toggle').click();
  await page.waitForFunction(()=>racingAudioTelemetry.ready&&!window.racingAudioError);
  assert.equal(await page.evaluate(()=>Object.keys(racingAudioAssets).length),13);report.checks.push('failed sound download is visible and retry completes without duplicate imports');
  await page.locator('#audio-settings summary').click();await page.locator('#start').click();
  await page.waitForFunction(()=>racingState.phase===2&&racingAudioTelemetry.active);
  const cdp=await context.newCDPSession(page),{windowId}=await cdp.send('Browser.getWindowForTarget');
  await cdp.send('Browser.setWindowBounds',{windowId,bounds:{windowState:'minimized'}});
  let visibilityAvailable=true;
  try {await page.waitForFunction(()=>document.hidden,{},{polling:100,timeout:5000});}
  catch(e){if(e.name!=='TimeoutError')throw e;visibilityAvailable=false;}
  if(visibilityAvailable){
   await page.waitForTimeout(300);
   const hidden=await page.evaluate(()=>({hidden:document.hidden,a:EngineAudioDevices.get(racingAudioDevice).snapshot(),clock:EngineAudioDevices.get(racingAudioDevice).context.currentTime}));
   assert(hidden.hidden&&!hidden.a.active);
   await page.waitForTimeout(300);assert(Math.abs(await page.evaluate(()=>EngineAudioDevices.get(racingAudioDevice).context.currentTime)-hidden.clock)<.04);
  }else report.limitations=['Automation host does not expose a hidden document after browser minimization; real visibility transition remains unverified.'];
  await cdp.send('Browser.setWindowBounds',{windowId,bounds:{windowState:'normal'}});
  await page.bringToFront();
  if(visibilityAvailable)await page.waitForFunction(()=>racingState.paused&&!racingAudioTelemetry.active);
  if(await page.evaluate(()=>racingState.paused))await page.locator('#resume').click();
  await page.waitForFunction(()=>racingAudioTelemetry.active);
  if(visibilityAvailable)report.checks.push('minimized browser window freezes audio clock; foreground remains paused until explicit resume');
  await page.keyboard.down('w');await page.waitForFunction(()=>racingState.speed>45);await page.keyboard.down('d');await page.keyboard.down('Space');
  await page.waitForFunction(()=>racingAudioTelemetry.starts[4]>0,{},{timeout:10000});
  for(const key of ['w','d','Space'])await page.keyboard.up(key);
  report.checks.push('real wheeled slide starts tire loop from grounded lateral velocity');
  report.telemetry=await page.evaluate(()=>racingAudioTelemetry);
  // Capture real device mix while the player accelerates, boosts and fires.
  await page.keyboard.press('r');await page.evaluate(()=>{
    const d=EngineAudioDevices.get(racingAudioDevice),stream=d.context.createMediaStreamDestination();d.output.connect(stream);
    const recorder=new MediaRecorder(stream.stream),chunks=[];recorder.ondataavailable=e=>chunks.push(e.data);
    window.audioRecording={recorder,stream,chunks};recorder.start();
  });
  for(const key of ['w','Shift','f'])await page.keyboard.down(key);await page.waitForTimeout(4500);
  for(const key of ['w','Shift','f'])await page.keyboard.up(key);
  const encoded=await page.evaluate(()=>new Promise(resolve=>{
    const r=audioRecording;r.recorder.onstop=async()=>{
      const data=new Uint8Array(await new Blob(r.chunks,{type:r.recorder.mimeType}).arrayBuffer());
      EngineAudioDevices.get(racingAudioDevice).output.disconnect(r.stream);r.stream.disconnect();
      let binary='';for(const v of data)binary+=String.fromCharCode(v);resolve(btoa(binary));
    };r.recorder.stop();
  }));
  await fs.writeFile(path.join(out,'gameplay-mix.webm'),Buffer.from(encoded,'base64'));
 }catch(e){report.failure=String(e.stack||e);throw e;}
 finally{await fs.writeFile(path.join(out,'lifecycle.json'),JSON.stringify(report,null,2));await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
