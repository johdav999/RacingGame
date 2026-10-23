'use strict';
(() => {
  const byId=id=>document.getElementById(id);
  const key='neon-ascension.performance.v1';
  const defaults=[75,100,100], names=['Low','Balanced','High'];
  let settings={preset:1,scale:100}, storageMessage='Saved on this browser';
  const valid=s=>s && Number.isInteger(s.preset) && s.preset>=0 && s.preset<=2 &&
    Number.isInteger(s.scale) && s.scale>=50 && s.scale<=100 && s.scale%5===0;
  try {
    const stored=localStorage.getItem(key);
    if(stored) { const parsed=JSON.parse(stored); if(valid(parsed))settings={preset:parsed.preset,scale:parsed.scale}; }
  } catch { storageMessage='Settings last this session only'; }
  const dialog=byId('performance-dialog'), preset=byId('quality-preset'), scale=byId('resolution-scale');
  const meter=new RacingFrameTimes();
  let lastPaint=0, returnFocus=null;
  window.EngineQuality={report(state){window.racingRenderState=state;}};
  window.resetRacingMeasurements=()=>{
    meter.reset();window.racingFrameTimes=null;lastPaint=0;
    byId('fps-counter').textContent='FPS —';
    byId('frame-measurements').textContent='Collecting frame times…';
  };
  function showSettings() {
    preset.value=settings.preset;scale.value=settings.scale;
    byId('resolution-value').textContent=settings.scale+'%';
    byId('quality-description').textContent=[
      'Lower pixel budget, shadows off, anti-aliasing off.',
      'Standard pixel density, up to one shadow map where supported, anti-aliasing off.',
      'Higher pixel density, up to four shadow maps and 4× anti-aliasing where supported.'
    ][settings.preset];
    byId('performance-save').textContent=storageMessage;
    window.racingQualitySettings={...settings};
  }
  window.applyRacingSettings=()=>{
    if(window.Module?._race_quality)Module._race_quality(settings.preset,settings.scale);
  };
  function change() {
    showSettings();window.applyRacingSettings();
    try {localStorage.setItem(key,JSON.stringify(settings));storageMessage='Saved on this browser';showSettings();}
    catch {storageMessage='Settings last this session only';showSettings();}
  }
  preset.addEventListener('change',()=>{settings={preset:Number(preset.value),scale:defaults[Number(preset.value)]};change();});
  scale.addEventListener('input',()=>{byId('resolution-value').textContent=scale.value+'%';});
  scale.addEventListener('change',()=>{settings.scale=Number(scale.value);change();});
  byId('performance-open').onclick=()=>{
    returnFocus=document.activeElement;
    if(window.racingState?.phase!==0 && !window.racingState?.paused)Module?._race_action?.(6);
    window.Module?._race_cancel?.();
    dialog.showModal();
  };
  byId('performance-close').onclick=()=>dialog.close();
  dialog.addEventListener('close',()=>{returnFocus?.focus({preventScroll:true});});
  byId('measurement-reset').onclick=()=>window.resetRacingMeasurements();
  document.addEventListener('visibilitychange',()=>window.resetRacingMeasurements());
  window.addEventListener('resize',()=>window.resetRacingMeasurements());
  window.recordRacingFrame=(now,cpu,active)=>{
    if(!active || document.hidden) {window.resetRacingMeasurements();return;}
    meter.record(now,cpu);
    if(now-lastPaint<500)return;
    lastPaint=now;
    if(dialog.open) {
      const r=window.racingRenderState;
      const applied=window.racingQualityApplied;
      byId('render-resolution').textContent=`${names[applied?.preset??settings.preset]} · ${window.racingBackend||'Starting'} · ${Module.canvas.width} × ${Module.canvas.height} render pixels${r?` · ${r.effectiveMsaaSamples}× samples${r.shadows?'':' · Shadows unavailable'}`:''}`;
    }
    const s=meter.summary();window.racingFrameTimes=s;
    if(!s)return;
    byId('fps-counter').textContent=`FPS ${Math.round(s.fps)} · ${s.meanMs.toFixed(1)} ms · p95 ${s.p95Ms.toFixed(1)} ms`;
    if(!dialog.open)return;
    byId('frame-measurements').textContent=`${s.meanMs.toFixed(1)} ms average · ${s.p95Ms.toFixed(1)} ms p95 · ${s.worstMs.toFixed(1)} ms worst\n${s.slowFrames} frames over 33.3 ms / ${s.frames} frames (${s.seconds.toFixed(1)} s)\nCPU callback ${s.cpuMs.toFixed(1)} ms average`;
  };
  showSettings();window.resetRacingMeasurements();
})();
