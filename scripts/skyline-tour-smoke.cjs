// Inspect the final built review UI on both real renderer backends.
const assert=require('node:assert/strict');
const fs=require('node:fs/promises');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8189';
const out='build/evidence/skyline-expansion';
(async()=>{
 const browser=await chromium.launch({headless:true,executablePath:process.env.BROWSER_EXECUTABLE||'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 const results={};
 try {for(const backend of ['webgpu','webgl2']) {
  const context=await browser.newContext({viewport:{width:1440,height:900}});
  const page=await context.newPage(),errors=[];
  page.on('pageerror',e=>errors.push(String(e)));
  page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
  await page.goto(`${base}/?renderer=${backend}&cityReview=11&skylineTour=1`);
  await page.waitForFunction(()=>window.racingReady||window.racingCarError,null,{timeout:180000});
  assert(await page.evaluate(()=>!!racingReady));
  assert.equal(await page.evaluate(()=>racingBackend),backend);
  assert.equal(await page.locator('#street-view option').count(),5);
  for(const id of [11,12,13,6,7]) {
   await page.locator('#street-view').selectOption(String(id));
   await page.waitForTimeout(1200);
   assert.equal(await page.evaluate(()=>racingCityReview),id);
   assert.equal(new URL(page.url()).searchParams.get('cityReview'),String(id));
   await page.screenshot({path:`${out}/${backend}-tour-${id}.png`});
  }
  await page.getByRole('link',{name:'BACK TO RACING →'}).click();
  await page.waitForFunction(()=>window.racingReady,null,{timeout:180000});
  assert(await page.locator('#start').isVisible());
  assert.equal(await page.locator('.street-tour').count(),0);
  assert.deepEqual(errors,[]);
  results[backend]={views:[11,12,13,6,7],returnToRace:true,errors};
  await context.close();console.log(backend+' skyline tour passed');
 }} finally {await fs.writeFile(`${out}/tour-results.json`,JSON.stringify(results,null,2));await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
