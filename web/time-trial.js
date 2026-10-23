'use strict';
// Each track/chassis owns one versioned record, independent of career saves.
const trialStorage=new Map();
const trialKey=(track,chassis)=>`neon-ascension.trial.v1.${track}.${chassis}`;
window.restoreTimeTrials=()=>{
  for(let track=0;track<2;track++)for(let chassis=0;chassis<4;chassis++){
    const key=trialKey(track,chassis),state={original:null,blocked:false,message:'Saved on this browser'};
    trialStorage.set(key,state);
    let pointer=0;
    try {
      state.original=localStorage.getItem(key);
      if(state.original===null)continue;
      if(state.original.length>1500000||/[^\x20-\x7e]/.test(state.original))throw Error('unreadable');
      const bytes=new TextEncoder().encode(state.original);pointer=Module._malloc(bytes.length);
      if(!pointer)throw Error('unreadable');
      Module.HEAPU8.set(bytes,pointer);
      if(!Module._race_load_trial(track,chassis,pointer,bytes.length))throw Error('unreadable');
    }catch(error){
      state.blocked=true;state.message=error.message==='unreadable'?'Save unreadable · original kept · session only':'Storage unavailable · session only';
    }finally{if(pointer)Module._free(pointer);}
  }
};
window.saveTimeTrial=(track,chassis,save)=>{
  const key=trialKey(track,chassis),state=trialStorage.get(key);
  if(!state||state.blocked)return;
  try {
    if(localStorage.getItem(key)!==state.original){state.blocked=true;state.message='Save changed in another tab · reload to save';return;}
    localStorage.setItem(key,save);state.original=save;
  }catch{state.blocked=true;state.message='Storage full or unavailable · best kept this session only';}
};
window.addEventListener('storage',event=>{
  for(const [key,state] of trialStorage)if((event.key===key||event.key===null)&&event.newValue!==state.original){
    state.blocked=true;state.message='Save changed in another tab · reload to save';
  }
});
$('mode-select').value=new URLSearchParams(location.search).get('mode')==='trial'?'trial':'race';
$('mode-select').onchange=event=>{
  const trial=event.target.value==='trial';command(trial?31:30);
  const url=new URL(location.href);url.searchParams.set('mode',trial?'trial':'race');history.replaceState(null,'',url);
  for(const link of document.querySelectorAll('.system a')){
    const target=new URL(url);target.searchParams.set('renderer',new URL(link.href).searchParams.get('renderer'));link.href=target;
  }
};
document.querySelectorAll('.watch-best').forEach(button=>button.onclick=()=>{command(32);driveFocus();});
function splitText(splits,index){return splits[index]?time(splits[index]-(index?splits[index-1]:0)):'—';}
window.paintTimeTrial=s=>{
  const t=s.trial;if(!t)return;
  document.body.classList.toggle('time-trial',t.active);
  $('mode-select').value=t.active?'trial':'race';
  $('trial-summary').hidden=!t.active;$('trial-hud').hidden=!t.active||s.phase===0||s.phase===3||s.paused;
  $('trial-results').hidden=!t.active;
  $('upgrades').hidden=t.active;$('upgrade-scope').hidden=t.active;
  $('best').previousElementSibling.textContent=t.active?'PERSONAL BEST':'BEST LAP';
  $('position').previousElementSibling.textContent=t.active?'MODE':'POSITION';
  document.querySelector('.event .tag').textContent=t.active?'TIME TRIAL':desertSelected?'HELIOS RUN':'STREET RACE';
  document.querySelector('.controls span:nth-child(7)').innerHTML=t.active?'<kbd>R</kbd> RECOVER / INVALIDATE':'<kbd>R</kbd> RECOVER +3s';
  document.querySelector('.intro').textContent=t.active?'Solo · 1 lap · 3 sectors · stock tuning. Boost allowed. Recovery or leaving the circuit invalidates the run.':'Race. Earn. Upgrade your machine. 3 laps · 6 racers · 300–1,200 CR per finish';
  const state=trialStorage.get(trialKey(desertSelected?1:0,s.chassis));
  $('trial-summary').textContent=`${chassisNames[s.chassis]} best: ${t.best?time(t.best):'No clean run yet'} · ${state?.message||'Loading records…'}`;
  document.querySelectorAll('.watch-best').forEach(button=>{button.hidden=!t.active;button.disabled=!window.racingReady||!t.best;});
  $('map-rivals').style.display=t.active?'none':'';
  $('map-gate').style.display=t.replay?'none':'';
  $('map-player').style.display=t.replay?'none':'';
  document.querySelector('.map-caption').innerHTML=t.replay?'CYAN / BEST RUN REPLAY':'<i></i> NEXT GATE <b>●</b> YOU';
  $('map-ghost').hidden=!t.active||!t.ghost;
  $('map-ghost').style.display=t.active&&t.ghost?'':'none';
  $('map-ghost').setAttribute('cx',t.x);$('map-ghost').setAttribute('cy',t.z);
  if(!t.active)return;
  $('standings').hidden=true;$('combat-panel').hidden=true;
  $('speed-panel').hidden=t.replay||s.phase===0||s.phase===3;
  $('position').textContent=t.replay?'REPLAY':'SOLO';
  if(t.replay)$('sector').textContent='BEST RUN REPLAY';
  $('lap').innerHTML='01<small>/01</small>';$('best').textContent=t.best?time(t.best):'—';
  if(window.racingReady)$('start').textContent='START TIME TRIAL →';
  $('trial-state').textContent=t.replay?'BEST RUN REPLAY':!t.clean?'INVALID RUN · RESTART TO SET A BEST':t.reference[2]?(t.ghost?'RACING YOUR BEST · CYAN GHOST':'GHOST FINISHED · KEEP GOING'):'CLEAN RUN · SET YOUR FIRST BEST';
  for(let i=0;i<3;i++){
    const delta=t.splits[i]&&t.reference[i]?t.splits[i]-t.reference[i]:null;
    const text=`S${i+1}  ${splitText(t.splits,i)}${delta===null?'':`  (${delta>=0?'+':'−'}${Math.abs(delta).toFixed(3)}s)`}`;
    $(`trial-sector-${i}`).textContent=text;
    $(`trial-sector-${i}`).className=delta===null?'':delta<=0?'ahead':'behind';
  }
  $('notice').hidden=t.clean||s.phase!==2||s.paused||t.replay;
  $('notice').textContent='RUN INVALID · RESTART FROM PAUSE MENU';
  $('classification-status').textContent='TIME TRIAL / ONE LAP / STOCK TUNING';
  $('result-title').textContent=t.replay?'REPLAY COMPLETE.':!t.clean?'RUN INVALID.':t.newBest?'NEW PERSONAL BEST.':'TRIAL COMPLETE.';
  $('result-detail').textContent=t.clean?'Sector times below. Signed deltas compare elapsed time at each sector line.':'Recovery, leaving the circuit or the 10-minute limit invalidated this attempt. Your previous best is safe.';
  $('trial-results').textContent=[0,1,2].map(i=>$(`trial-sector-${i}`).textContent).join(' · ');
  $('race-payout').textContent=t.replay?'Replay of your saved best run':`${t.best?'Personal best '+time(t.best):'No personal best yet'} · ${state?.message||''}`;
  if(t.replay){$('map-player').setAttribute('cx',t.x);$('map-player').setAttribute('cy',t.z);}
};
