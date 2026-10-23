'use strict';
(() => {
  const key='neon-ascension.difficulty.v1',select=document.getElementById('difficulty-select');
  let saved=true,last=null;
  window.readRacingDifficulty=()=>{
    let value=1;
    try{const stored=localStorage.getItem(key);if(['0','1','2'].includes(stored))value=Number(stored);}
    catch{saved=false;}
    select.value=String(value);last=value;return value;
  };
  select.onchange=()=>command(40+Number(select.value));
  window.paintRacingDifficulty=()=>{
    const value=window.racingAi.difficulty,trial=!!window.racingTrial?.active;
    select.value=String(value);select.disabled=trial||window.racingState?.phase!==0;
    if(value!==last&&!window.racingLeague?.racing){
      try{localStorage.setItem(key,String(value));saved=true;}catch{saved=false;}
      last=value;
    }
    document.getElementById('difficulty-note').textContent=trial?'Solo event · rival difficulty does not affect time trials.':
      ['Relaxed pace, earlier braking and more following space.','Balanced pace and braking margins.','Faster pace, later braking and closer racing.'][value]+(saved?'':' · session only');
  };
})();
