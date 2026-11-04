 'use strict';
(() => {
  const names=['Street League','District League','Metro League','Elite League','Master Championship'];
  const drivers=['YOU','AXIOM','NYX','KODA','ECHO','RIFT'];
  const tracks=['Kairox / Midnight Circuit','Desert Solar / Helios Run'];
  const dialog=$('league-dialog'),recordsDialog=$('league-records-dialog');let lastView='',requestedStart=false,lastRecords='';
  const pointsHeading=document.createElement('th');pointsHeading.textContent='PTS';pointsHeading.className='league-points';
  document.querySelector('#result .classification thead tr').append(pointsHeading);
  for(let id=0;id<6;id++){const cell=document.createElement('td');cell.className='league-points';cell.id=`finish-points-${id}`;$(`finish-${id}`).append(cell);}
  const garage=()=>window.racingState?.phase===0;
  const show=()=>{if(!dialog.open)dialog.showModal();};
  $('league-open').onclick=show;
  $('league-close').onclick=()=>dialog.close();
  // Return from completed results to the garage without starting another race.
  $('league-results-open').onclick=()=>{command(7);show();};
  $('league-restart').onclick=()=>{const c=window.racingLeague;if(c&&garage())command(50+c.league);};
  const continueLeague=(direct=false)=>{
    const c=window.racingLeague;
    if(!c?.canRace||!garage()||!window.racingReady)return;
    // Persisted points live outside the track scene. The normal loader only
    // loads the scheduled environment and retains renderer/quality settings.
    if(c.track!==selectedTrackId){
      if(!window.racingCareer?.saved){$('league-message').textContent='Saving is unavailable. Continue on the next track after browser storage is restored.';return;}
      const url=new URL(location.href);url.searchParams.set('track',c.track?'desert':'kairox');url.searchParams.delete('mode');
      if(direct){url.searchParams.set('continueLeague',String(c.revision));url.searchParams.delete('career');}
      else {url.searchParams.set('career','1');url.searchParams.delete('continueLeague');}
      for(const key of [...url.searchParams.keys()])if(key.endsWith('Review'))url.searchParams.delete(key);
      location.assign(url);return;
    }
    requestedStart=true;command(60); // Focus only once the C++ runtime accepts the start.
  };
  $('league-continue').onclick=()=>continueLeague();
  $('league-resume').onclick=()=>continueLeague(true);
  for(let i=0;i<5;i++){
    const button=document.createElement('button');button.type='button';button.className='league-tier';button.id=`league-tier-${i}`;
    button.onclick=()=>{const c=window.racingLeague;if(!c||!garage())return;
      if(c.league!==i||!c.canRace)command(50+i);
    };$('league-ladder').append(button);
  }
  const select=$('league-record-select');
  names.forEach((name,id)=>{const option=document.createElement('option');option.value=id;option.textContent=name;select.append(option);});
  $('league-records-open').onclick=()=>{
    if(window.racingLeague?.league>=0)select.value=String(racingLeague.league);
    lastRecords='';paintRecords(window.racingState,window.racingLeague);recordsDialog.showModal();
  };
  $('league-records-close').onclick=()=>recordsDialog.close();
  select.onchange=()=>{lastRecords='';paintRecords(window.racingState,window.racingLeague);};
  function paintRecords(s,c){
    if(!s||!c)return;
    $('league-records-save').textContent=s.career?.saveMessage||'Loading results…';
    const league=Number(select.value),record=c.records?.[league];
    const current=c.league===league&&c.completed>0?c:null;
    const key=JSON.stringify([league,current,record]);if(key===lastRecords)return;lastRecords=key;
    const cards=$('league-record-cards');cards.replaceChildren();
    const add=(title,run)=>{
      if(!run)return;
      const card=document.createElement('section');card.className='league-record-card';
      const heading=document.createElement('h3');heading.textContent=title;card.append(heading);
      const player=run.standings.find(row=>row.id===0),summary=document.createElement('p');
      const state=['Not entered','In progress','Qualified for final','Season complete','Final retry available',run.league===4?'Master champion':'League champion','Season complete'][run.status];
      summary.className='league-record-summary';summary.textContent=`${state} · YOU: ${player.position}/6 · ${player.points} points · ${run.completed}/${run.rounds} rounds`+
        (run.finalAttempts?` · Final position: ${run.finalPosition||'DNF'} (${run.finalAttempts} attempt${run.finalAttempts===1?'':'s'})`:'');card.append(summary);
      const wrap=document.createElement('div');wrap.className='league-table-wrap';
      const table=document.createElement('table');table.className='classification';table.setAttribute('aria-label',`${title} standings`);
      const head=table.createTHead().insertRow();for(const name of ['POS','DRIVER','PTS','WINS','ROUND POSITIONS']){const th=document.createElement('th');th.textContent=name;head.append(th);}
      const body=table.createTBody();for(const row of run.standings){const tr=body.insertRow();if(row.id===0)tr.className='you';
        for(const value of [row.position,drivers[row.id],row.points,row.wins,row.results.map(p=>p||'DNF').join(' · ')])tr.insertCell().textContent=value;}
      wrap.append(table);card.append(wrap);cards.append(card);
    };
    add('Current season',current);
    if(record?.latest&&record.latest.revision!==current?.revision)add('Latest saved run',record.latest);
    add('Best saved run',record?.best);
    if(!cards.children.length){const p=document.createElement('p');p.textContent='No saved results for this league yet. Complete a round to start your record.';cards.append(p);}
  }
  window.paintChampionship=s=>{
    const c=window.racingLeague;if(!c)return;
    s.championship=c;
    $('league-resume').hidden=!c.canRace;
    $('league-resume').disabled=s.phase!==0||!window.racingReady||(!s.career?.saved&&c.track!==selectedTrackId);
    $('league-resume').textContent=`CONTINUE LEAGUE · ${c.final?'FINAL':`ROUND ${c.completed+1}`} →`;
    $('league-resume').title=`${names[c.league]} · ${tracks[c.track]}`;
    paintRecords(s,c);
    const intent=new URLSearchParams(location.search).get('continueLeague');
    if(intent!==null&&window.racingReady&&s.phase===0){
      const url=new URL(location.href);url.searchParams.delete('continueLeague');history.replaceState(null,'',url);
      if(intent===String(c.revision)&&c.canRace&&c.track===selectedTrackId){requestedStart=true;command(60);}
      else show();
    }
    if(requestedStart&&c.racing&&s.phase!==0){requestedStart=false;driveFocus();}

    const isGarage=s.phase===0,active=c.racing&&s.phase!==0;
    $('league-summary').textContent=c.league<0?'Start in Street League. Earn promotion to the Master Championship.':
      `${names[c.league]} · ${c.status===5?'Champion':c.status===2||c.status===4?'Promotion final':`${c.completed}/${c.rounds} rounds complete`} · ${c.trophies===31?'Master title earned':`Unlocked: ${names[c.unlocked]}`}`;
    $('league-save').textContent=s.career?.saveMessage||'Loading save…';
    $('league-save').classList.toggle('save-warning',!s.career?.saved);
    $('league-hud').hidden=!active||s.paused||s.phase===3;
    $('league-hud').textContent=active?`${names[c.league]} · ${c.final?'PROMOTION FINAL':`ROUND ${c.completed+1} / ${c.rounds}`}`:'';
    $('league-results-open').hidden=!active||s.phase!==3;
    $('league-results-open').disabled=!s.classificationFinal;
    $('league-result').hidden=!active||s.phase!==3;
    const player=c.standings.find(r=>r.id===0);
    const messages=[ '',`Round ${c.completed+1} of ${c.rounds}. Top three qualify for the promotion final.`,
      'Final qualified! Win the promotion final to unlock the next league.',
      'Season complete. Finish in the top three next season to qualify. Your credits and upgrades are kept.',
      'Final not won. Retry the final without repeating your season.',
      c.league===4?'MASTER CHAMPION! You won the championship on total points.':`PROMOTED! ${names[Math.min(c.league+1,4)]} is now unlocked.`,
      'Master Championship complete. Start another season to challenge for the title.' ];
    let message=messages[c.status];
    if(c.league===4&&c.status===1)message=`Round ${c.completed+1} of 6. Highest total points after six rounds wins the Master Championship.`;
    const resultPosition=s.standings?.find(r=>r.id===0)?.position;
    const earned=c.raceFinal?'Promotion final':`+${[0,15,12,10,8,6,4][resultPosition]||0} championship points`;
    $('league-result').textContent=s.classificationFinal?`${earned}. ${message} You: ${player.points} pts · championship position ${player.position}/6.`:'Championship points pending final race classification.';
    document.querySelector('#result .restart').hidden=active;
    document.querySelectorAll('.league-points').forEach(cell=>cell.hidden=!active);
    for(const row of s.standings||[])$(`finish-points-${row.id}`).textContent=
      !s.classificationFinal?'…':c.raceFinal?'—':String([0,15,12,10,8,6,4][row.position]||0);
    if(dialog.open&&!isGarage){dialog.close();driveFocus();}
    if(recordsDialog.open&&!isGarage)recordsDialog.close();
    if(active){$('difficulty-select').disabled=true;$('difficulty-note').textContent=`${names[c.league]} sets rival difficulty and equipment.`;}
    if(new URLSearchParams(location.search).has('career')&&isGarage&&!window.leagueAutoOpened){window.leagueAutoOpened=true;show();}
    const view=JSON.stringify([c,isGarage,window.racingReady,!!s.career?.saved]);
    if(view===lastView)return;lastView=view;
    for(let i=0;i<5;i++){
      const button=$(`league-tier-${i}`),won=!!(c.trophies&(1<<i)),locked=i>c.unlocked;
      button.disabled=locked||!isGarage||!window.racingReady;
      button.classList.toggle('current',i===c.league);
      button.setAttribute('aria-pressed',String(i===c.league));
      button.textContent=`${i+1}. ${names[i]} · ${locked?'Locked — win '+names[i-1]:won?'Champion · replay available':i===c.league&&c.canRace?'Season in progress':'Enter season'}`;
      button.title=locked?`Win ${names[i-1]} to unlock`:i===c.league?'Current league':`Start ${names[i]}. The current run is saved to your results; earned titles and upgrades are kept.`;
    }
    $('league-season').hidden=c.league<0;if(c.league<0)return;
    $('league-season-title').textContent=names[c.league];$('league-message').textContent=message+(!s.career?.saved&&c.track!==selectedTrackId?' Track travel needs working browser storage; your current session progress is kept.':'');
    const caps=['Stock (rank 0)','Rank 1','Rank 2','Rank 3','Rank 3'];
    $('league-rules').textContent=`3 laps · 6 racers · ${['Easy','Normal','Normal','Hard','Hard'][c.league]} rivals · Equipment ceiling: ${caps[c.league]}. Owned upgrades above the ceiling are temporarily capped. Rivals use the ceiling equipment.`;
    $('league-schedule').replaceChildren();
    for(let r=0;r<c.rounds;r++){
      const item=document.createElement('li');item.textContent=`${r<c.completed?'✓ ':''}Round ${r+1}: ${tracks[(r+c.league%2)%2]}`;
      if(r===c.completed)item.className='current';$('league-schedule').append(item);
    }
    if(c.league<4){const item=document.createElement('li');item.textContent=`Promotion final: ${tracks[c.league%2]} · top three qualify · six-racer field`;$('league-schedule').append(item);}
    $('league-rows').replaceChildren();
    for(const row of c.standings){const tr=document.createElement('tr');if(row.id===0)tr.className='you';
      for(const value of [row.position,drivers[row.id],row.points,row.gap?`−${row.gap}`:'—',row.wins,row.results.map(p=>p||'DNF').join(' · ')||'—']){
        const td=document.createElement('td');td.textContent=value;tr.append(td);}$('league-rows').append(tr);}
    $('league-continue').hidden=!c.canRace;
    $('league-continue').disabled=!isGarage||!window.racingReady||(!s.career?.saved&&c.track!==selectedTrackId);
    $('league-continue').textContent=`${c.track!==selectedTrackId?'LOAD '+tracks[c.track].split(' / ')[0].toUpperCase():c.final?'RACE PROMOTION FINAL':'RACE ROUND '+(c.completed+1)} →`;
    $('league-restart').disabled=!isGarage||!window.racingReady;
    $('league-restart').textContent=c.status===3||c.status===6?'START NEW SEASON':c.status===5?'REPLAY THIS LEAGUE':'RESTART SEASON · SAVE THIS RUN';
  };
})();
