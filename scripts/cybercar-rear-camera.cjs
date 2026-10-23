// Capture the actual, unmodified chase camera on both backends.
const {chromium}=require('playwright');
const assert=require('node:assert/strict');
const fs=require('node:fs/promises');
const path=require('node:path');
(async()=>{
 const out=process.env.EVIDENCE_DIR||'build/evidence/cybercar-rear/driving';
 await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,executablePath:process.env.BROWSER_EXECUTABLE||'C:/Program Files/Google/Chrome/Application/chrome.exe'});
 const report={};
 try{for(const backend of ['webgpu','webgl2']){
  const page=await browser.newPage({viewport:{width:1440,height:900}});
  const errors=[];page.on('pageerror',e=>errors.push(String(e)));
  page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
  await page.goto(`${process.env.DEMO_URL||'http://127.0.0.1:8091'}/?renderer=${backend}&track=${process.env.TRACK||'kairox'}`);
  await page.waitForFunction(()=>window.racingReady,null,{timeout:120000});
  assert.equal(await page.evaluate(()=>racingBackend),backend);
  await page.locator('#car').click();await page.locator('#start').click();
  await page.waitForFunction(()=>racingState.phase===2);
  await page.screenshot({path:path.join(out,`${backend}-grid.png`)});
  await page.keyboard.down('w');await page.waitForFunction(()=>racingState.speed>35);
  await page.screenshot({path:path.join(out,`${backend}-driving.png`)});
  // Let the normal chase rig leave the station overhang before the second view.
  await page.waitForFunction(()=>racingState.elapsed>4.5);
  await page.screenshot({path:path.join(out,`${backend}-open-stretch.png`)});
  report[backend]=await page.evaluate(()=>({backend:racingBackend,state:racingState,wheels:racingWheels,asset:racingCarAsset}));
  report[backend].errors=errors;
  assert.deepEqual(errors,[]);
  await page.keyboard.up('w');await page.close();
 }}finally{await browser.close();await fs.writeFile(path.join(out,'camera.json'),JSON.stringify(report,null,2));}
})();
