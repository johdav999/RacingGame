// Final mount-fit captures after visual-only changes; run after weapon-smoke, never concurrently.
const assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8104',out=path.resolve('build/evidence/weapon-final');
(async()=>{
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 const report={browser:await browser.version(),backends:[]};
 try{for(const backend of ['webgpu','webgl2']){
  const context=await browser.newContext({viewport:{width:1440,height:900},storageState:{cookies:[],origins:[{origin:new URL(base).origin,localStorage:[{name:'neon-ascension.career.v1',value:'NA2 10000 5 0 0 3 0 0 3 0 0'}]}]}});
  const page=await context.newPage(),errors=[];page.on('pageerror',e=>errors.push(String(e)));page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
  await page.goto(`${base}/?renderer=${backend}&weaponReview=1`);
  await page.waitForFunction(()=>window.racingCarError||(window.racingReady&&window.racingWeaponVisual?.loaded),null,{timeout:120000});
  const loadingError=await page.evaluate(()=>window.racingCarError);
  if(loadingError){console.error('Scene load:',loadingError);await page.screenshot({path:path.join(out,`${backend}-load-error.png`)});throw new Error(loadingError);}
  assert.equal(await page.evaluate(()=>racingBackend),backend);
  for(const [id,button,name] of [[0,'car','vektor'],[2,'bastion','bastion'],[1,'hover','skimmer'],[3,'vector','vector']]){
   await page.locator('#'+button).click();await page.waitForFunction(id=>racingState.chassis===id,id);
   assert.equal(await page.evaluate(()=>racingWeaponVisual.rank),3);
   await page.screenshot({path:path.join(out,`${backend}-${name}-mounted.png`)});
   console.log(backend,name,'mounted');
  }
  await page.locator('#start').click();await page.waitForFunction(()=>racingState.phase===2);
  await page.keyboard.down('f');await page.waitForFunction(()=>racingWeaponVisual.recoil>.02);
  await page.screenshot({path:path.join(out,`${backend}-vector-fire.png`)});await page.keyboard.up('f');
  assert.deepEqual(errors,[]);
  report.backends.push({backend,errors,asset:await page.evaluate(()=>racingWeaponAssets[0]),build:await page.evaluate(async()=>await(await fetch('build-info.json')).json())});
  await context.close();
 }}finally{await fs.writeFile(path.join(out,'report.json'),JSON.stringify(report,null,2));await browser.close();}
})().catch(e=>{console.error(e);process.exit(1);});
