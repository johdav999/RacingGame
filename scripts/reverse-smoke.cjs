// Physical reverse acceptance on the exact generated RacingGame site.
const assert=require('node:assert/strict');
const {chromium}=require('playwright');
const base=process.env.DEMO_URL||'http://127.0.0.1:8088';

(async()=>{
 const browser=await chromium.launch({headless:true,executablePath:process.env.BROWSER_EXECUTABLE||'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 try{
  for(const backend of ['webgpu','webgl2'])for(const [button,chassis] of [['car',0],['bastion',2]]){
   const page=await browser.newPage({viewport:{width:1280,height:800}});
   const errors=[];page.on('pageerror',error=>errors.push(String(error)));
   await page.goto(`${base}/?renderer=${backend}`);
   await page.waitForFunction(()=>window.racingReady&&window.racingState,{},{timeout:60000}).catch(async error=>{
    console.error(backend,button,await page.evaluate(()=>({status:document.getElementById('status')?.textContent,
     ready:window.racingReady,carError:window.racingCarError,asset:window.racingCarAsset,
     state:window.racingState?.phase})),errors);throw error;
   });
   assert.equal(await page.evaluate(()=>window.racingBackend),backend);
   await page.locator(`#${button}`).click();
   await page.waitForFunction(selected=>racingState.chassis===selected,chassis);
   await page.locator('#start').click();
   await page.waitForFunction(()=>racingState.phase===2);
   const start=await page.evaluate(()=>racingState);
   await page.keyboard.down('s');await page.waitForTimeout(700);await page.keyboard.up('s');
   const end=await page.evaluate(()=>racingState);
   const backward=(end.x-start.x)*Math.sin(start.heading)+(end.z-start.z)*Math.cos(start.heading);
   assert(backward<-.1,`${backend} ${button}: S must move the car backward (travel ${backward})`);
   assert.deepEqual(errors,[],`${backend} ${button}: browser errors`);
   console.log(`${backend} ${button}: reversed ${(-backward).toFixed(2)} m`);
   await page.close();
  }
 }finally{await browser.close();}
})().catch(error=>{console.error(error);process.exitCode=1;});
