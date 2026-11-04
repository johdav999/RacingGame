// Synthetic codec records verify browser storage keys; physical saved laps are
// covered by racing_desert_trials. This runs in a fresh, isolated browser profile.
const assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8142',out=path.resolve('build/evidence/desert-circuits');
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:process.env.BROWSER_EXECUTABLE||'C:/Program Files/Google/Chrome/Application/chrome.exe',args:['--force-high-performance-gpu']});
 const report=[];
 try {for(const backend of ['webgpu','webgl2']) {
  const page=await browser.newPage(),errors=[];page.setDefaultTimeout(180000);
  page.on('pageerror',e=>errors.push(String(e)));
  await page.addInitScript(()=>{
   for(const [id,duration,z] of [[4,3,-170],[5,6,-205]]) {
    const key=`neon-ascension.trial.v1.${id}.0`,count=duration*20+1;
    let save=`NATT1 ${id} 0 ${duration} ${duration/3} ${duration*2/3} ${duration} ${count}`;
    for(let i=0;i<count;i++)save+=` ${i*.05} ${i*.1} 1 ${z} 0 0 0`;
    if(localStorage.getItem(key)===null)localStorage.setItem(key,save);
   }
  });
  for(const [slug,duration] of [['sirocco',3],['canyon',6]]) {
   await page.goto(`${base}/?renderer=${backend}&track=${slug}&mode=trial`);
   await page.waitForFunction(()=>window.racingReady||window.racingCarError);
   assert(!await page.evaluate(()=>window.racingCarError));
   assert.equal(await page.evaluate(()=>racingBackend),backend);
   assert.equal(await page.evaluate(()=>racingState.trial.best),duration);
   await page.locator('.watch-best').first().click();await page.waitForFunction(()=>racingState.trial.replay);
   await page.waitForFunction(()=>racingState.phase===3);
   assert.equal(await page.evaluate(()=>racingState.trial.best),duration);
   report.push({backend,slug,best:duration,replay:true});
  }
  assert.deepEqual(errors,[]);await page.close();
 }}finally{await browser.close();await fs.writeFile(path.join(out,'storage-report.json'),JSON.stringify(report,null,2));}
 console.log('Distinct stored personal bests and replay passed on both renderers');
})().catch(e=>{console.error(e);process.exitCode=1;});
