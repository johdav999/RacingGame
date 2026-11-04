'use strict';
const $ = id => document.getElementById(id);
const canvas = $('viewport');
const circuitCatalog=[
  {id:0,key:'kairox',name:'Kairox / Midnight Circuit',label:'STREET RACE'},
  {id:1,key:'desert',name:'Desert Solar / Helios Run',label:'HELIOS RUN',region:'HELIOS',note:'Solar-field straight · sandstone canyon · conduit corridor'},
  {id:4,key:'sirocco',name:'Sirocco Solar / Dune Sweep',label:'DUNE SWEEP',region:'SIROCCO',note:'Long solar straight · broad mesa sweep · offset dune return'},
  {id:5,key:'canyon',name:'Canyon Relay / Twin Mesa',label:'TWIN MESA',region:'CANYON RELAY',note:'Linked canyon bends · northern crown · blue conduit return'}
];
const selectedCircuit=circuitCatalog.find(t=>t.key===new URLSearchParams(location.search).get('track'))||circuitCatalog[0];
const selectedTrackId=selectedCircuit.id;
const desertSelected=['desert','sirocco','canyon'].includes(selectedCircuit.key);
window.racingTrackKind=selectedCircuit.key;
$('track-select').value=window.racingTrackKind;
$('track-select').onchange=event=>{
  const url=new URL(location.href);url.searchParams.set('track',event.target.value);
  for(const key of [...url.searchParams.keys()])if(key.endsWith('Review'))url.searchParams.delete(key);
  location.assign(url);
};
for(const link of document.querySelectorAll('.system a')){
  const url=new URL(location.href);url.searchParams.set('renderer',new URL(link.href).searchParams.get('renderer'));link.href=url;
}
if(desertSelected){
  document.body.classList.add('desert');document.title='Neon Ascension · '+selectedCircuit.name;
  document.querySelector('.brand small').textContent=selectedCircuit.region+' / SOLAR COMPLEX';
  document.querySelector('.event').innerHTML='<span class="live-dot"></span> '+selectedCircuit.name.split(' / ')[0].toUpperCase()+' <span class="tag">'+selectedCircuit.label+'</span>';
  document.querySelector('.garage-heading .eyebrow').textContent=selectedCircuit.region+' / GARAGE';
  document.querySelector('.map-panel > div > span').textContent=selectedCircuit.region;
  canvas.setAttribute('aria-label',canvas.getAttribute('aria-label').replace('Kairox city',selectedCircuit.name));
  $('track-note').textContent=selectedCircuit.note;
}

const chassisNames=['Vektor','Skimmer','Bastion','Vector'];
const chassisButtons=['car','hover','bastion','vector'];
const careerKey='neon-ascension.career.v1';
let savedCareer=null, publishedCareer='', saveBlocked=false, saveMessage='SAVED ON THIS BROWSER';
window.readCareer=()=>{
  try {savedCareer=localStorage.getItem(careerKey);return savedCareer;}
  catch {saveBlocked=true;saveMessage='Storage unavailable · progress lasts this session only';return null;}
};
window.careerInvalid=()=>{
  saveBlocked=true;saveMessage='Save unreadable · original kept · this session only';
};
window.addEventListener('storage',event=>{
  if((event.key===careerKey||event.key===null)&&event.newValue!==savedCareer){
    saveBlocked=true;saveMessage='Save changed in another tab · reload to continue saving';
  }
});
window.updateCareer=(save,stats)=>{
  if(save!==publishedCareer){
    if(!saveBlocked)try {
      if(localStorage.getItem(careerKey)!==savedCareer){
        saveBlocked=true;saveMessage='Save changed in another tab · reload to continue saving';
      }else if(save!==savedCareer){localStorage.setItem(careerKey,save);savedCareer=save;}
    }catch {saveBlocked=true;saveMessage='Storage unavailable · progress lasts this session only';}
    publishedCareer=save;
  }
  const fields=save.split(' | ')[0].split(' ').slice(1).map(Number);
  window.racingCareer={credits:fields[0],completedRaces:fields[1],selected:fields[2],
    vehicles:[fields.slice(3,6),fields.slice(6,9)],chassis:fields[9]??fields[2],paint:save.split(" / ")[1]?.split(" ").map(Number)??Array(4).fill(0x1000000),...stats,saved:!saveBlocked,saveMessage};
};
const upgradeNames=['Drive system','Pulse cannon','Energy shield'];
for(let i=0;i<3;i++){
  const card=document.createElement('div');card.className='upgrade';
  card.innerHTML=`<div><strong>${upgradeNames[i]}</strong><span id="rank-${i}"></span></div><p id="effect-${i}"></p><button id="buy-${i}" class="upgrade-buy" disabled></button>`;
  $('upgrades').append(card);
  $(`buy-${i}`).onclick=()=>command(20+i);
}
const held = new Map();
let mapReady = false, lastPaint = 0, lastPhase = -1, lastOrder = '';
let carHash='', carLoading=false;
const vehicleQueue=[];
let vehicleLoading=false;
window.racingVehicleAssets={};
window.racingWeaponAssets={};
window.queueWeaponArt=(id,name,hash)=>{vehicleQueue.push({id,name,hash,weapon:true});loadNextVehicle();};
window.racingDesertAssets={};
window.queueDesertArt=(id,name,hash)=>{vehicleQueue.push({id,name,hash,desert:true});loadNextVehicle();};
window.racingTrackAssets={};
window.racingPlazaAssets={};
window.racingGateAssets={};
window.queueGateMarker=(name,hash)=>{vehicleQueue.push({id:0,name,hash,gateMarker:true});loadNextVehicle();};
window.queuePlazaArt=(id,name,hash)=>{vehicleQueue.push({id,name,hash,plaza:true});loadNextVehicle();};
window.queueTrackArt=(id,name,hash)=>{vehicleQueue.push({id,name,hash,trackKit:true});loadNextVehicle();};
window.racingBuildingAssets={};
window.racingStreetAssets={};
window.queueStreetArt=(id,name,hash)=>{vehicleQueue.push({id,name,hash,street:true});loadNextVehicle();};
window.racingCityKitAssets={};
window.queueCityKitArt=(id,name,hash)=>{vehicleQueue.push({id,name,hash,cityKit:true});loadNextVehicle();};
window.queueVehicleArt=(id,name,hash)=>{vehicleQueue.push({id,name,hash});loadNextVehicle();};
window.queueBuildingArt=(id,name,hash)=>{vehicleQueue.push({id,name,hash,building:true});loadNextVehicle();};
async function loadNextVehicle(){
  if(vehicleLoading||!window.racingCarAsset?.loaded)return;
  vehicleLoading=true;
  try {
    while(vehicleQueue.length){
      const asset=vehicleQueue[0];let pointer=0;
      try {
        $('status').textContent=`LOADING ${asset.name.toUpperCase()}…`;
        window.racingCarError='';
        const response=await fetch(`assets/models/${asset.desert?(asset.name.startsWith('dune-building-')?'dune-villages/':asset.name.startsWith('desert-race-')?'desert-race-kit/':'desert-solar/'):asset.weapon?'weapons/':asset.gateMarker?'navigation/':asset.plaza?'plaza/':asset.trackKit?'track-kit/':asset.street?'street-kit/':asset.cityKit?'city-kit/':asset.building?'buildings/':''}${asset.name}/${asset.name}.glb`);
        if(!response.ok)throw new Error(`${asset.name} download failed (${response.status})`);
        const bytes=new Uint8Array(await response.arrayBuffer());
        const hash=Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',bytes)),b=>b.toString(16).padStart(2,'0')).join('');
        if(hash!==asset.hash)throw new Error(`${asset.name} asset is out of date. Rebuild the game.`);
        pointer=Module._malloc(bytes.length);if(!pointer)throw new Error('Insufficient memory to load scene');
        Module.HEAPU8.set(bytes,pointer);
        const install=asset.desert?Module._race_load_desert_art:asset.weapon?Module._race_load_weapon:asset.gateMarker?Module._race_load_gate_marker:asset.plaza?Module._race_load_plaza_art:asset.trackKit?Module._race_load_track_art:asset.street?Module._race_load_street:asset.cityKit?Module._race_load_city_kit:asset.building?Module._race_load_building:Module._race_load_vehicle;
        if(!install(asset.id,pointer,bytes.length))throw new Error(window.racingCarError||`${asset.name} import failed`);
        (asset.desert?window.racingDesertAssets:asset.weapon?window.racingWeaponAssets:asset.gateMarker?window.racingGateAssets:asset.plaza?window.racingPlazaAssets:asset.trackKit?window.racingTrackAssets:asset.street?window.racingStreetAssets:asset.cityKit?window.racingCityKitAssets:asset.building?window.racingBuildingAssets:window.racingVehicleAssets)[asset.id]={loaded:true,name:asset.name,sha256:hash,bytes:bytes.length};
        vehicleQueue.shift();
      }finally{if(pointer)Module._free(pointer);}
    }
  }catch(error){window.racingCarError=String(error.message||error);$('status').textContent='SCENE UNAVAILABLE / '+window.racingCarError;}
  finally{vehicleLoading=false;}
}
window.loadPlayerCar=async hash=>{
  if(carLoading || window.racingCarAsset?.loaded)return;
  carHash=hash;carLoading=true;window.racingCarError='';
  window.racingCarAsset={loaded:false,url:'assets/models/cybercar-clean-detail/cybercar.glb'};
  let pointer=0;
  try {
    $('status').textContent='LOADING CYBERCAR…';
    const response=await fetch(window.racingCarAsset.url);
    if(!response.ok)throw new Error(`Car download failed (${response.status})`);
    const bytes=new Uint8Array(await response.arrayBuffer());
    const digest=Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',bytes)),
      b=>b.toString(16).padStart(2,'0')).join('');
    if(digest!==hash)throw new Error('Car asset does not match its part contract. Rebuild the game.');
    pointer=Module._malloc(bytes.length);
    if(!pointer)throw new Error('Insufficient memory to load the car');
    Module.HEAPU8.set(bytes,pointer);
    if(!Module._race_load_car(pointer,bytes.length))throw new Error(window.racingCarError||'Car import failed');
    window.racingCarAsset={...window.racingCarAsset,loaded:true,sha256:digest,bytes:bytes.length};
  } catch(error) {
    window.racingCarError=String(error.message||error);
    $('status').textContent='CAR UNAVAILABLE / '+window.racingCarError;
  } finally {
    if(pointer)Module._free(pointer);
    carLoading=false;
    if(window.racingCarAsset?.loaded)loadNextVehicle();
  }
};
// Stable rows keep the result buttons and keyboard focus in place as rivals finish.
for(let id=0;id<6;id++){
  const row=document.createElement('li');row.id=`racer-${id}`;
  row.innerHTML='<b></b><span></span><small></small>';$('leaderboard').append(row);
  const result=document.createElement('tr');result.id=`finish-${id}`;
  result.innerHTML='<td></td><td></td><td></td>';$('result-rows').append(result);
  if(id){const dot=document.createElementNS('http://www.w3.org/2000/svg','circle');
    dot.id=`rival-${id}`;dot.setAttribute('r','3.5');dot.setAttribute('fill','#7bf4e5');$('map-rivals').append(dot);}
}
function command(id) { cancel(); if(window.Module?._race_action)Module._race_action(id); }
function cancel() { held.clear(); if(window.Module?._race_cancel)Module._race_cancel(); }
function driveFocus() { canvas.focus({preventScroll:true}); }
const bindings = {
  KeyW:[100,1],ArrowUp:[100,1],KeyS:[102,1],ArrowDown:[102,1],
  KeyA:[101,-1],ArrowLeft:[101,-1],KeyD:[101,1],ArrowRight:[101,1],
  Space:[103,1],ShiftLeft:[104,1],ShiftRight:[104,1],KeyX:[100,-1],KeyF:[105,1],KeyE:[106,1]
};
const sources = new Map(Object.keys(bindings).map((key,index)=>[key,index+1]));
window.addEventListener('keydown',event=>{
  if($('performance-dialog').open)return;
  if(event.repeat)return;
  if(event.code==='Escape' && window.racingState?.phase!==0) { event.preventDefault();command(5);driveFocus();return; }
  if(event.code==='KeyR' && document.activeElement===canvas) {event.preventDefault();command(4);return;}
  const binding=bindings[event.code];
  if(!binding || document.activeElement!==canvas || !window.racingReady || window.racingState?.paused)return;
  event.preventDefault();held.set(event.code,binding);
  Module._race_input(binding[0],sources.get(event.code),binding[1]);
});
window.addEventListener('keyup',event=>{
  const binding=held.get(event.code);if(!binding)return;
  event.preventDefault();held.delete(event.code);Module._race_input(binding[0],sources.get(event.code),0);
});
window.addEventListener('blur',()=>command(6));
document.addEventListener('visibilitychange',()=>{if(document.hidden)command(6);});
canvas.addEventListener('blur',cancel);
canvas.addEventListener('pointerdown',driveFocus);
$('paint-preview').onclick=()=>{
  const active=document.body.classList.toggle('paint-preview');
  $('paint-preview').textContent=active?'BACK TO GARAGE':'VIEW CAR';
  $('paint-preview').setAttribute('aria-pressed',String(active));
};
let paintTimer;
$('paint-color').addEventListener('input',()=>{
  clearTimeout(paintTimer);
  const chassis=window.racingState?.chassis,color=parseInt($('paint-color').value.slice(1),16);
  $('paint-value').value=$('paint-color').value.toUpperCase();
  paintTimer=setTimeout(()=>{if(window.racingState?.phase===0&&window.racingState.chassis===chassis)Module._race_paint(chassis,color);},60);
});
$('paint-color').addEventListener('change',()=>{
  clearTimeout(paintTimer);if(window.racingState?.phase===0)Module._race_paint(window.racingState.chassis,parseInt($('paint-color').value.slice(1),16));
});
$('paint-reset').onclick=()=>{clearTimeout(paintTimer);if(window.racingState?.phase===0)Module._race_paint(window.racingState.chassis,0x1000000);};
$('car').onclick=()=>command(2);
$('hover').onclick=()=>command(3);
$('bastion').onclick=()=>command(12);
$('vector').onclick=()=>command(13);
$('start').onclick=()=>{
  if(!window.racingReady){if(window.racingCarError){if(window.racingCarAsset?.loaded)loadNextVehicle();else window.loadPlayerCar(carHash);}return;}
  command(1);driveFocus();
};
$('resume').onclick=()=>{command(5);driveFocus();};
document.querySelectorAll('.restart').forEach(button=>button.onclick=()=>{command(1);driveFocus();});
document.querySelectorAll('.garage-button').forEach(button=>button.onclick=()=>command(7));
function time(seconds) {
  const ms=Math.round(seconds*1000),mins=Math.floor(ms/60000),secs=Math.floor(ms/1000)%60;
  return `${String(mins).padStart(2,'0')}:${String(secs).padStart(2,'0')}.${String(ms%1000).padStart(3,'0')}`;
}
window.updateRaceUI = s => {
  const now=performance.now();
  // The engine calls this once after each render; count before throttling HUD updates.
  if(now-lastPaint<65 && s.phase===lastPhase)return;lastPaint=now;
  if(!mapReady && window.racingTrack) {
    const xs=racingTrack.map(p=>p[0]),zs=racingTrack.map(p=>p[1]);
    const minX=Math.min(...xs)-20,minZ=Math.min(...zs)-20;
    $('map').setAttribute('viewBox',`${minX} ${minZ} ${Math.max(...xs)-minX+20} ${Math.max(...zs)-minZ+20}`);
    const path='M'+racingTrack.map(p=>p.join(',')).join('L')+'Z';
    $('map-road').setAttribute('d',path);$('map-line').setAttribute('d',path);mapReady=true;
  }
  document.body.classList.toggle('in-race',s.phase!==0);
  $('garage').hidden=s.phase!==0;
  $('race-hud').hidden=s.phase===0;
  $('standings').hidden=s.phase===0 || s.phase===3 || s.paused;
  $('combat-panel').hidden=s.phase!==2 || s.paused;
  $('speed-panel').hidden=s.phase===0 || s.phase===3;
  $('pause-panel').hidden=!s.paused || s.phase===0;
  $('result').hidden=s.phase!==3 || s.paused;
  $('countdown').hidden=s.phase!==1 || s.paused;
  const count=String(Math.max(1,Math.ceil(s.countdown)));if($('countdown').textContent!==count)$('countdown').textContent=count;
  chassisButtons.forEach((id,index)=>{$(id).classList.toggle('selected',s.chassis===index);$(id).setAttribute('aria-pressed',String(s.chassis===index));});
  $('vehicle-note').textContent=[
    'Brake before corners. Release the slide to regain grip.',
    'Turn early. Countersteer to catch the glide; brake to tighten your line.',
    'Heavy armor and a stronger drive. Brake early and commit to your line.',
    'Fast hover drive. Turn early and control the glide through tight corners.'
  ][s.chassis];
  $('upgrade-scope').textContent=s.kind===0?'Wheeled upgrades apply to Vektor and Bastion.':'Hover upgrades apply to Skimmer and Vector.';
  const career=s.career;
  if(career){
    if(s.phase!==0&&document.body.classList.contains('paint-preview')){
      document.body.classList.remove('paint-preview');$('paint-preview').textContent='VIEW CAR';$('paint-preview').setAttribute('aria-pressed','false');
    }
    const paint=career.paint[s.chassis],original=paint===0x1000000;
    if(document.activeElement!==$('paint-color'))$('paint-color').value=original?'#ffffff':'#'+paint.toString(16).padStart(6,'0');
    if(document.activeElement!==$('paint-color'))$('paint-value').value=original?'ORIGINAL':$('paint-color').value.toUpperCase();
    $('paint-chassis').textContent=chassisNames[s.chassis].toUpperCase();
    $('paint-color').disabled=!window.racingReady||s.phase!==0;
    $('paint-reset').disabled=!window.racingReady||s.phase!==0||original;
    $('credits').textContent=career.credits.toLocaleString();
    $('save-status').textContent=career.saveMessage;
    $('save-status').classList.toggle('save-warning',!career.saved);
    const ranks=career.vehicles[s.kind];
    const effects=[
      level=>`+${level*10}% power & boost · +${level*7.2} km/h limit`,
      level=>`Mk ${level+1} · ${(.8-level*.1).toFixed(1)}s between shots`,
      level=>`${100+25*level} energy · ${18+3*level}/s recharge`
    ];
    for(let i=0;i<3;i++){
      const level=ranks[i],cost=career.prices[i],button=$(`buy-${i}`);
      $(`rank-${i}`).textContent=`${level} / 3`;
      $(`effect-${i}`).textContent=effects[i](level)+(level<3?' → '+effects[i](level+1):'');
      button.disabled=!window.racingReady||level===3||career.credits<cost;
      button.textContent=level===3?'MAX LEVEL':career.credits<cost?`${cost} CR · NEED ${cost-career.credits} MORE`:`UPGRADE · ${cost} CR`;
      button.setAttribute('aria-label',`${upgradeNames[i]}, level ${level} of 3. ${button.textContent}`);
    }
    $('race-payout').textContent=s.classificationFinal?`+${career.reward.toLocaleString()} CR EARNED · ${career.credits.toLocaleString()} CR TOTAL`+(career.saved?' · SAVED':' · SESSION ONLY'):'Winnings pending · waiting for all six racers';
  }
  document.querySelectorAll('.restart,.garage-button').forEach(button=>button.disabled=s.phase===3&&!s.classificationFinal);
  $('start').disabled=!window.racingReady&&!window.racingCarError;
  $('start').textContent=window.racingReady?'ENTER CIRCUIT →':window.racingCarError?(vehicleQueue[0]?.gateMarker?'RETRY BEACON DOWNLOAD':vehicleQueue[0]?.trackKit?'RETRY TRACK DOWNLOAD':window.racingCarAsset?.loaded?((vehicleQueue[0]?.plaza||vehicleQueue[0]?.building||vehicleQueue[0]?.cityKit||vehicleQueue[0]?.street)?'RETRY CITY DOWNLOAD':'RETRY VEHICLE DOWNLOAD'):'RETRY CAR DOWNLOAD'):'LOADING CIRCUIT…';
  $('lap').innerHTML=`${String(Math.min(s.lap+1,3)).padStart(2,'0')}<small>/03</small>`;
  $('time').textContent=time(s.elapsed);$('best').textContent=s.best?time(s.best):'—';
  $('speed').textContent=Math.round(s.speed);$('charge').textContent=`${Math.round(s.battery)}%`;
  $('charge-bar').style.width=s.battery+'%';$('speed-panel').classList.toggle('boosting',s.boosting);
  $('vehicle-name').textContent=`${chassisNames[s.chassis].toUpperCase()} / ${s.kind===0?'WHEELED':'HOVERCRAFT'}`;
  $('sector').textContent=s.gate===0?'FINISH GATE':`GATE ${s.gate} / 12`;
  $('map-player').setAttribute('cx',s.x);$('map-player').setAttribute('cy',s.z);
  const gate=window.racingTrack?.[s.gate*16];if(gate){$('map-gate').setAttribute('cx',gate[0]);$('map-gate').setAttribute('cy',gate[1]);}
  $('notice').hidden=s.valid || s.phase!==2 || s.paused;
  $('notice').textContent='OFF CIRCUIT / RECOVERING +3s';
  const racers=s.standings||[],you=racers.find(r=>r.id===0);
  const c=s.combat;
  if(c){
    $('weapon-state').textContent=c.cooldown>0?`CHARGING ${c.cooldown.toFixed(1)}s`:'PULSE READY';
    $('shield-state').textContent=c.shield?'SHIELD ACTIVE':c.releaseRequired?'RELEASE E TO RESET':c.charge<25?'RECHARGING':'SHIELD READY';
    const shieldPercent=Math.round(c.charge/(career?.capacity||100)*100);
    $('shield-charge').textContent=`${shieldPercent}%`;
    $('shield-meter').style.width=shieldPercent+'%';
    $('combat-panel').classList.toggle('shield-active',c.shield);
    $('combat-panel').classList.toggle('disrupted',c.disruption>0);
    const target=racers.find(r=>r.id===s.target);
    $('target-name').textContent=c.disruption>0?'DRIVE DISRUPTED':target?`TARGET / ${target.name}`:'AIM AT A RIVAL AHEAD';
    $('combat-tally').textContent=`${c.hits} HITS · ${c.blocks} BLOCKS`;
    $('result-combat').textContent=`Pulse: ${c.shots} shots / ${c.hits} hits · Shield: ${c.blocks} blocks`;
  }
  if(you)$('position').innerHTML=`${you.position}<small>/6</small>`;
  const order=racers.map(r=>r.id).join(',');
  for(const r of racers){
    const row=$(`racer-${r.id}`),cells=row.children,result=$(`finish-${r.id}`),rc=result.children;
    row.classList.toggle('you',r.id===0);result.classList.toggle('you',r.id===0);
    cells[0].textContent=r.position;cells[1].textContent=r.name;
    cells[2].textContent=r.finished?'FIN':r.combat?.shield?'SHIELD':r.kind===0?'CAR':'HOVER';
    rc[0].textContent=r.position;rc[1].textContent=`${r.name} / ${chassisNames[r.chassis].toUpperCase()}`;
    rc[2].textContent=r.finished?time(r.elapsed)+(r.recoveries?` (+${r.recoveries*3}s)`:''):`LAP ${Math.min(3,r.lap+1)} / 3`;
    if(order!==lastOrder){$('leaderboard').append(row);$('result-rows').append(result);}
    if(r.id){$(`rival-${r.id}`).setAttribute('cx',r.x);$(`rival-${r.id}`).setAttribute('cy',r.z);}
  }
  lastOrder=order;
  $('classification-status').textContent=s.classificationFinal?'FINAL CLASSIFICATION / 3 LAPS':'LIVE CLASSIFICATION / RACERS STILL FINISHING';
  $('result-title').textContent=s.classificationFinal&&you?.position===1?'YOU WON.':'RACE FINISHED.';
  if(s.phase===3){$('result-time').textContent=time(s.elapsed);$('result-detail').textContent=`Best lap ${time(s.best)} · ${s.recoveries} recoveries (+${s.recoveries*3}s included)`;}
  window.paintTimeTrial?.(s);
  window.paintChampionship?.(s);
  if(s.phase===3 && lastPhase!==3)$('result-title').setAttribute('tabindex','-1');
  if(s.phase===3 && lastPhase!==3)$('result-title').focus();
  lastPhase=s.phase;
};
window.engineStatus=message=>{$('status').textContent=message;};
window.engineStats=(meshes,lights,layers)=>{window.racingStats={meshes,lights,layers};};
window.enginePerformance=snapshot=>{window.racingPerformance=snapshot;};
var Module={
  locateFile:(name,prefix)=>prefix+name+(window.racingAssetVersions?.[name]?'?v='+window.racingAssetVersions[name]:''),
  canvas,
  printErr:message=>console.error(message),
  onAbort:message=>{$('status').textContent='ENGINE ERROR / '+message;$('start').textContent='ENGINE UNAVAILABLE';}
};
window.addEventListener('error',event=>{$('status').textContent='ERROR / '+event.message;});
