'use strict';
(() => {
  const key='neon-ascension.audio.v1',defaults={master:.65,engines:.7,effects:.8,cues:.85,muted:false};
  let settings={...defaults},device,loading=false,error='',saved=true;
  const queue=[];window.racingAudioAssets={};
  try {
    const value=JSON.parse(localStorage.getItem(key));
    if(value&&['master','engines','effects','cues'].every(k=>Number.isFinite(value[k])&&value[k]>=0&&value[k]<=1)&&typeof value.muted==='boolean')settings=value;
  } catch {saved=false;}
  const element=id=>document.getElementById(id);
  function apply(persist=false){
    window.Module?._race_audio_volumes?.(settings.master,settings.engines,settings.effects,settings.cues,settings.muted?1:0);
    if(persist)try{localStorage.setItem(key,JSON.stringify(settings));saved=true;}catch{saved=false;}
  }
  for(const name of ['master','engines','effects','cues']) {
    const input=element('audio-'+name);input.value=Math.round(settings[name]*100);
    input.nextElementSibling.value=input.value+'%';
    input.oninput=()=>{settings[name]=Number(input.value)/100;input.nextElementSibling.value=input.value+'%';apply(true);};
  }
  window.initRaceAudio=id=>{device=window.EngineAudioDevices.get(id);window.racingAudioDevice=id;apply();};
  window.queueRaceAudio=(id,name,hash)=>{queue.push({id,name,hash});void load();};
  async function load(){
    if(loading||!device)return;loading=true;error='';window.racingAudioError='';
    try {
      while(queue.length) {
        const a=queue[0],response=await fetch(`assets/audio/racing/${a.name}.wav`);
        if(!response.ok)throw Error(`Could not load ${a.name}`);
        const bytes=await response.arrayBuffer();
        const hash=Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',bytes)),b=>b.toString(16).padStart(2,'0')).join('');
        if(hash!==a.hash)throw Error('Audio asset mismatch. Reload the current build.');
        const pcm=await device.decode(new File([bytes],`${a.name}.wav`,{type:'audio/wav'}));
        let pointer=0;
        try {
          pointer=Module._malloc(pcm.samples.byteLength);if(!pointer)throw Error('Audio memory unavailable');
          new Float32Array(Module.HEAPU8.buffer,pointer,pcm.samples.length).set(pcm.samples);
          if(!Module._race_load_audio(a.id,pcm.sampleRate,pcm.channels,pointer,pcm.samples.length))throw Error('Audio import failed');
          window.racingAudioAssets[a.id]={name:a.name,sha256:hash,seconds:pcm.samples.length/pcm.sampleRate,loaded:true};
          queue.shift();
        }finally{if(pointer)Module._free(pointer);}
      }
    }catch(e){error=e.message;window.racingAudioError=error;}
    finally{loading=false;window.paintRaceAudio();}
  }
  element('audio-toggle').onclick=()=>{
    if(error){void load();return;}
    if(settings.muted||(!device?.active&&!window.racingState?.paused)){settings.muted=false;window.Module?._race_audio_activate?.();}
    else settings.muted=true;
    apply(true);window.paintRaceAudio();
  };
  window.paintRaceAudio=()=>{
    const state=window.racingAudioTelemetry,snapshot=device?.snapshot();
    const paused=window.racingState?.paused||document.hidden;
    const text=error?'Audio unavailable · retry':!state?.ready?`Loading sounds ${state?.loaded||0}/13`:
      paused?'Paused':settings.muted?'Muted':snapshot?.active?'Sound on':'Click Enable sound or resume the race';
    element('audio-status').textContent=text+(saved?'':' · settings not saved');
    element('audio-toggle').textContent=error?'RETRY AUDIO':settings.muted?'UNMUTE':snapshot?.active||paused?'MUTE':'ENABLE SOUND';
    element('audio-toggle').setAttribute('aria-pressed',settings.muted?'true':'false');
    window.racingAudioSettings={...settings,saved};
  };
  document.addEventListener('pointerdown',event=>{
    if(!event.target.closest('#audio-settings')&&!document.hidden)window.Module?._race_audio_activate?.();
  },{capture:true});
  document.addEventListener('keydown',event=>{
    if(!event.repeat&&!event.target.closest('#audio-settings')&&!document.hidden)window.Module?._race_audio_activate?.();
  },{capture:true});
  for(const name of ['visibilitychange','pagehide'])window.addEventListener(name,()=>{
    if(name==='pagehide'||document.hidden)window.Module?._race_audio_suspend?.();
  });
  // Sound controls must not leak Space/arrow interaction into driving input.
  element('audio-settings').addEventListener('keydown',event=>event.stopPropagation());
  element('audio-settings').addEventListener('toggle',()=>{
    if(element('audio-settings').open&&window.racingState?.phase>0&&!window.racingState.paused){
      window.Module?._race_audio_suspend?.();window.Module?._race_action?.(6);
    }
  });
})();
