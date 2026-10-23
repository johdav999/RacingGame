const assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8112',out=path.resolve('build/evidence/race-audio');
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files/Google/Chrome/Application/chrome.exe',args:['--force-high-performance-gpu']});
 const report={browser:await browser.version(),backends:[]};
 try {for(const backend of ['webgpu','webgl2']) {
  const context=await browser.newContext({viewport:{width:1440,height:900}}),page=await context.newPage(),errors=[];
  page.setDefaultTimeout(180000);page.on('pageerror',e=>errors.push(String(e)));
  page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
  const ready=async()=>{await page.waitForFunction(()=>window.racingCarError||window.racingAudioError||(window.racingReady&&window.racingAudioTelemetry?.ready));assert(!await page.evaluate(()=>window.racingCarError||window.racingAudioError));};
  const snapshot=()=>page.evaluate(()=>({state:racingState,audio:racingAudioTelemetry,device:EngineAudioDevices.get(racingAudioDevice).snapshot()}));
  const signal=()=>page.evaluate(async()=>{
    const device=EngineAudioDevices.get(racingAudioDevice),analyser=device.context.createAnalyser(),sink=device.context.createGain();analyser.fftSize=2048;device.output.connect(analyser);sink.gain.value=0;analyser.connect(sink);sink.connect(device.context.destination);const start=device.context.currentTime;
    let square=0,peak=0,count=0;const samples=new Float32Array(2048);
    for(let i=0;i<12;++i){await new Promise(r=>setTimeout(r,25));analyser.getFloatTimeDomainData(samples);for(const v of samples){square+=v*v;peak=Math.max(peak,Math.abs(v));count++;}}
    device.output.disconnect(analyser);analyser.disconnect();sink.disconnect();return {rms:Math.sqrt(square/count),peak,clock:device.context.currentTime-start,state:device.snapshot()};
  });
  await page.goto(`${base}/?renderer=${backend}&track=desert`);await ready();
  assert.equal((await snapshot()).audio.active,false,'No sound before a user gesture');
  assert.equal((await snapshot()).audio.loaded,13);
  const result={backend,errors,checks:[],vehicles:[],assets:await page.evaluate(()=>racingAudioAssets),build:await page.evaluate(async()=>await(await fetch('build-info.json')).json())};
  assert.equal(await page.evaluate(()=>racingBackend),backend);assert(!result.build.engineDirty&&!result.build.developmentEngine);
  result.wasmSha256=await page.evaluate(async()=>Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',await(await fetch('racing.wasm')).arrayBuffer())),b=>b.toString(16).padStart(2,'0')).join(''));
  await page.locator('#audio-settings summary').click();await page.locator('#audio-toggle').click();
  await page.waitForFunction(()=>racingAudioTelemetry.active&&racingAudioTelemetry.voices===1);
  await page.waitForTimeout(750);
  const audible=await signal();assert(audible.rms>.0005&&audible.peak<.99,JSON.stringify(audible));result.garageSignal=audible;
  await page.locator('#audio-toggle').click();await page.waitForTimeout(150);assert((await signal()).rms<.00001,'Mute really silences the output');
  await page.locator('#audio-toggle').click();await page.waitForTimeout(150);assert((await signal()).rms>.0005);
  await page.locator('#audio-engines').fill('55');await page.locator('#audio-effects').fill('60');
  await page.screenshot({path:path.join(out,`${backend}-settings.png`)});
  await page.locator('#audio-settings summary').click();
  result.checks.push('13 verified real assets','no autoplay','measured nonzero device output','mute silence','independent volume controls');
  for(const [id,name] of ['car','hover','bastion','vector'].entries()) {
    await page.locator('#'+name).click();await page.waitForTimeout(250);
    const before=await snapshot();await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);
    const started=await snapshot();assert(started.audio.starts[10]>=before.audio.starts[10]+3);assert.equal(started.audio.starts[11],before.audio.starts[11]+1);
    const idle=started.audio.pitch;await page.keyboard.down('w');await page.keyboard.down('Shift');await page.keyboard.down('e');await page.keyboard.down('f');
    await page.waitForFunction(()=>racingState.speed>30&&racingAudioTelemetry.starts[5]>0&&racingAudioTelemetry.starts[6]>0&&racingAudioTelemetry.starts[7]>0);
    const moving=await snapshot();assert(moving.audio.pitch>idle+.04,'Engine pitch responds to real acceleration');
    const output=await signal();assert(output.rms>.0005&&output.peak<.99);
    const spatial=await page.evaluate(()=>Array.from(EngineAudioDevices.get(racingAudioDevice).voices.values()).filter(v=>v.options.spatial).map(v=>v.options.position));assert(spatial.length>=5);
    for(const key of ['w','Shift','e','f'])await page.keyboard.up(key);
    await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused&&!racingAudioTelemetry.active);
    const clock=await page.evaluate(()=>EngineAudioDevices.get(racingAudioDevice).context.currentTime);await page.waitForTimeout(250);
    assert(Math.abs(await page.evaluate(()=>EngineAudioDevices.get(racingAudioDevice).context.currentTime)-clock)<.04,'Paused audio clock freezes');
    const shots=(await snapshot()).audio.starts[6];await page.locator('#resume').click();await page.waitForFunction(()=>racingAudioTelemetry.active);
    // The controlled player is not holding F after resume; AI can still fire.
    await page.keyboard.press('r');await page.waitForFunction(()=>racingState.recoveries>0);
    await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);await page.locator('#pause-panel .garage-button').click();
    await page.waitForFunction(()=>racingState.phase===0&&racingAudioTelemetry.active&&racingAudioTelemetry.voices===1);
    result.vehicles.push({name,idle,pitch:moving.audio.pitch,output,spatialVoices:spatial.length});console.log(backend,name,'audio passed');
  }
  // A real host page-lifecycle event, independent of RAF/scene simulation.
  await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);
  await page.evaluate(()=>window.dispatchEvent(new PageTransitionEvent('pagehide')));await page.waitForFunction(()=>!racingAudioTelemetry.active);
  const hidden=await page.evaluate(()=>EngineAudioDevices.get(racingAudioDevice).context.currentTime);await page.waitForTimeout(250);
  assert(Math.abs(await page.evaluate(()=>EngineAudioDevices.get(racingAudioDevice).context.currentTime)-hidden)<.04);
  await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);await page.locator('#pause-panel .garage-button').click();await page.waitForFunction(()=>racingState.phase===0);
  await page.locator('#track-select').selectOption('kairox');await page.waitForURL('**track=kairox');await ready();
  assert.equal(await page.evaluate(()=>racingAudioSettings.engines),.55);assert.equal(await page.evaluate(()=>racingAudioSettings.effects),.6);
  await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2&&racingAudioTelemetry.active);assert((await signal()).rms>.0005);
  await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
  await page.setViewportSize({width:390,height:844});await page.locator('#audio-settings summary').click();
  assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));await page.screenshot({path:path.join(out,`${backend}-narrow.png`)});
  result.checks.push('four chassis pitch and real combat output','spatial opponents','countdown and go','paused device clock','pagehide suspension','recovery and garage cleanup','track-switch settings persistence','Kairox audio','narrow layout');
  assert.deepEqual(errors,[]);report.backends.push(result);await fs.writeFile(path.join(out,'report.json'),JSON.stringify(report,null,2));await context.close();
 }}catch(e){report.failure=String(e.stack||e);throw e;}
 finally{await fs.writeFile(path.join(out,'report.json'),JSON.stringify(report,null,2));await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
