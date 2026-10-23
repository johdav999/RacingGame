const assert=require('node:assert/strict');
const fs=require('node:fs/promises');
const {chromium}=require('playwright');
(async()=>{
 const base=process.env.DEMO_URL||'http://127.0.0.1:8189';
 const out='build/evidence/street-kit';await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:process.env.BROWSER_EXECUTABLE||'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 const results={};
 try{for(const renderer of ['webgpu','webgl2']){
  const page=await browser.newPage({viewport:{width:1440,height:900}}),errors=[];
  page.on('pageerror',e=>errors.push(String(e)));
  page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
  await page.goto(`${base}/?renderer=${renderer}&streetReview=0`);
  await page.waitForFunction(()=>window.racingReady||window.racingCarError,null,{timeout:120000});
  assert.equal(await page.evaluate(()=>!!window.racingReady),true);
  assert.equal(await page.evaluate(()=>racingBackend),renderer);
  const hashes=new Set();
  for(let id=0;id<6;id++){
   await page.locator('#street-view').selectOption(String(id));
   await page.waitForTimeout(600);
   assert.equal(await page.evaluate(()=>racingStreetReview),id);
   const screenshot=await page.screenshot({path:`${out}/${renderer}-tour-${id}.png`});
   hashes.add(require('node:crypto').createHash('sha256').update(screenshot).digest('hex'));
  }
  assert.equal(hashes.size,6);
  await page.setViewportSize({width:390,height:844});
  await page.waitForTimeout(200);
  assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),true);
  await page.screenshot({path:`${out}/${renderer}-tour-mobile.png`});
  await page.getByRole('link',{name:'BACK TO RACING →'}).click();
  await page.waitForFunction(()=>window.racingReady,null,{timeout:120000});
  assert.equal(await page.locator('#garage').isVisible(),true);
  assert.deepEqual(errors,[]);
  results[renderer]={views:6,returnToRace:true,narrowViewport:true,errors};
  await page.close();console.log(renderer+' in-game model tour passed');
 }}finally{await browser.close();await fs.writeFile(`${out}/tour-results.json`,JSON.stringify(results,null,2));}
})().catch(e=>{console.error(e);process.exitCode=1;});
