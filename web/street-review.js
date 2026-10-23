'use strict';
// Optional in-world tour; ordinary race URLs keep their existing camera and HUD.
if(new URLSearchParams(location.search).has('streetReview')||new URLSearchParams(location.search).has('skylineTour')) {
  const skyline=new URLSearchParams(location.search).has('skylineTour');
  const key=skyline?'cityReview':'streetReview';
  const views=skyline?['Service utility room','Residential mid-rise','Office mid-rise','Reused stepped tower','Reused crowned tower']:['Transit underpass','Station landmark','Apartment shopfronts','Side-street safety fence','Spectator railing','Service-depot frontage'];
  const ids=skyline?[11,12,13,6,7]:[0,1,2,3,4,5];
  document.body.classList.add('street-review');
  const panel=document.createElement('section');
  panel.className='street-tour';panel.setAttribute('aria-label','New Kairox models');
  panel.innerHTML='<p class="eyebrow">KAIROX / NEW CITY MODELS</p><h1>Explore the additions.</h1><label for="street-view">Location</label><select id="street-view"></select><p>'+(skyline?'Detailed service architecture and textured buildings throughout the skyline.':'12 Tripo models, placed throughout the circuit.')+'</p><a class="secondary" href="'+location.pathname+'?renderer='+encodeURIComponent(new URLSearchParams(location.search).get('renderer')||'webgpu')+'">BACK TO RACING →</a>';
  const select=panel.querySelector('select');
  views.forEach((name,index)=>select.add(new Option(name,String(ids[index]))));
  const initial=Number(new URLSearchParams(location.search).get(key));
  select.value=String(ids.includes(initial)?initial:ids[0]);
  const state=skyline?'racingCityReview':'racingStreetReview';
  window[state]=Number(select.value);
  select.onchange=()=>{
    window[state]=Number(select.value);
    const url=new URL(location.href);url.searchParams.set(key,select.value);history.replaceState(null,'',url);
  };
  document.body.append(panel);
}
