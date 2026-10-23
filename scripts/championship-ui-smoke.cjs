// Focused checks for final UI refinements, without replaying complete races.
const assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8135';
(async()=>{
 const browser=await chromium.launch({headless:true,executablePath:process.env.BROWSER_EXECUTABLE||'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 const report={backends:{},errors:[]};
 try{for(const backend of ['webgpu','webgl2']){
  const context=await browser.newContext({viewport:{width:1280,height:900},storageState:{cookies:[],origins:[{origin:new URL(base).origin,localStorage:[{name:'neon-ascension.difficulty.v1',value:'1'}]}]}});
  const page=await context.newPage();page.on('pageerror',e=>report.errors.push(String(e)));
  await page.goto(`${base}/?renderer=${backend}`);
  await page.waitForFunction(()=>window.racingReady&&window.racingState?.championship,{},{timeout:120000});
  await page.evaluate(()=>Module._race_quality(0,50));
  assert.equal(await page.evaluate(()=>racingBackend),backend);
  await page.locator('#league-open').click();await page.locator('#league-tier-0').click();
  await page.waitForFunction(()=>racingLeague.league===0);await page.locator('#league-continue').click();
  await page.waitForFunction(()=>racingState.phase===2);
  assert.equal(await page.evaluate(()=>document.activeElement.id),'viewport');
  await page.keyboard.down('w');await page.waitForFunction(()=>racingState.speed>8,{},{timeout:15000});await page.keyboard.up('w');
  assert.equal(await page.evaluate(()=>racingAi.difficulty),0);
  assert.equal(await page.evaluate(()=>localStorage.getItem('neon-ascension.difficulty.v1')),'1');
  assert.equal(await page.evaluate(()=>racingState.career.capacity),100);
  // Presentation-only fixture. It never submits a race result to C++ or storage.
  const rows=await page.evaluate(()=>{
   const s={...racingState,phase:3,classificationFinal:true};
   window.paintChampionship(s);
   return s.standings.map(row=>({position:row.position,text:document.getElementById(`finish-points-${row.id}`).textContent}));
  });
  for(const row of rows)assert.equal(row.text,String([0,15,12,10,8,6,4][row.position]));
  assert.equal(await page.evaluate(()=>racingLeague.completed),0,'Presentation fixture cannot score');
  await page.keyboard.press('Escape');await page.waitForFunction(()=>racingState.paused);
  await page.locator('#pause-panel .garage-button').click();await page.waitForFunction(()=>racingState.phase===0);
  assert.equal(await page.evaluate(()=>racingAi.difficulty),1);
  await page.reload();await page.waitForFunction(()=>window.racingReady&&window.racingState?.championship,{},{timeout:120000});
  assert.equal(await page.evaluate(()=>racingAi.difficulty),1);
  report.backends[backend]=['immediate keyboard driving focus','real W-key acceleration','league equipment telemetry','per-driver result points presentation fixture','quick-race difficulty preserved through league and reload'];
  await context.close();console.log(backend+' championship UI refinements passed');
 }assert.deepEqual(report.errors,[]);}finally{
  await fs.mkdir('build/evidence/championship',{recursive:true});
  await fs.writeFile('build/evidence/championship/ui-results.json',JSON.stringify(report,null,2));await browser.close();
 }
})().catch(e=>{console.error(e);process.exitCode=1;});
