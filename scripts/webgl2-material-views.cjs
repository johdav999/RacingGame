const {chromium}=require('playwright');const fs=require('node:fs');const assert=require('node:assert/strict');
const out=(process.env.EVIDENCE_DIR||'build/evidence/webgl2-material-fix/views');fs.mkdirSync(out,{recursive:true});
(async()=>{
 const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files/Google/Chrome/Application/chrome.exe'}),report={};
 try{for(const backend of ['webgl2','webgpu'])for(const variant of ['before','after']){
  const url=variant==='before'?(process.env.BEFORE_URL||'http://127.0.0.1:8190'):(process.env.AFTER_URL||'http://127.0.0.1:8197');
  const context=await browser.newContext({viewport:{width:1440,height:900}}),page=await context.newPage(),errors=[];page.on('pageerror',e=>errors.push(String(e)));page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
  for(const [name,key,id] of [['skyline','cityReview',13],['underpass','streetReview',0],['damp-plaza','plazaReview',0],['paving','plazaReview',2]]){
   await page.goto(url+'/?renderer='+backend+'&'+key+'='+id);
   await page.waitForFunction(()=>window.racingReady||window.racingCarError,null,{timeout:180000});assert(await page.evaluate(()=>!!racingReady));await page.addStyleTag({content:'#garage {visibility:hidden!important;}'});await page.waitForTimeout(1200);
   await page.screenshot({path:out+'/'+backend+'-'+variant+'-'+name+'.png'});
   report[backend+'-'+variant+'-'+name]={placements:await page.evaluate(()=>racingCityDetails.length),errors:[...errors]};assert.equal(report[backend+'-'+variant+'-'+name].placements,164);assert.deepEqual(errors,[]);
  }
  await context.close();fs.writeFileSync(out+'/results.json',JSON.stringify(report,null,2));console.log(backend,variant,'four actual static views passed');
 }}finally{await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
