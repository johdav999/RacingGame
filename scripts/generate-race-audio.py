"""Generate the explicitly requested racing sound set; no retries or overwrites."""
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ASSETS = [
    ('vektor-engine',6,True,'Seamless steady loop of a futuristic sports coupe engine at medium RPM, tight throaty mechanical combustion growl with subtle electric turbine whine. Constant speed and volume, dry isolated close recording, no acceleration sweep, no gear changes, no voices, no music, no other vehicles.'),
    ('skimmer-engine',6,True,'Seamless steady loop of a lightweight science fiction hovercraft lift engine, smooth airy magnetic turbine hum with soft granular air cushion hiss. Stable medium pitch, constant speed and volume, dry isolated close recording, no movement or whoosh pass, no voices, no music.'),
    ('bastion-engine',6,True,'Seamless steady loop of a heavy armored racing car engine at medium RPM, deep muscular industrial motor rumble with a restrained supercharger whine. Constant stable load and pitch, dry isolated close recording, no acceleration or gear changes, no voices, no music.'),
    ('vector-engine',6,True,'Seamless steady loop of a high performance antigravity racing craft, precise bright electromagnetic motor whir and smooth twin turbine harmonics, energetic but not piercing. Constant pitch and volume, dry isolated close recording, no sweep, no voices, no music.'),
    ('tire-slide',4,True,'Seamless continuous loop of racing tires scrubbing and sliding on dry asphalt, controlled rubber friction hiss with a soft low squeal, steady intensity, close dry recording. No engine, no crash, no voices, no music.'),
    ('reactor-boost',4,True,'Seamless continuous loop of a futuristic vehicle reactor boost, powerful smooth compressed plasma rush with a deep turbine undertone, sustained steady thrust. No start or end burst, no explosions, no voices, no music.'),
    ('laser-fire',0.8,False,'One short crisp futuristic vehicle laser cannon shot, sharp electric crack followed by a bright descending plasma zap and a very short tail. Single isolated shot, immediate attack, dry close sound, no repeated firing, no voices, no music.'),
    ('shield-on',1.1,False,'One compact futuristic energy shield activation, quick rising electronic shimmer opening into a soft resonant forcefield pulse, short clean decay. Immediate start, isolated dry game effect, no voices, no music.'),
    ('shield-impact',0.8,False,'One laser striking an energy shield, crisp electrical impact with glassy resonant ripple and short low electromagnetic thump, immediate attack, compact clean decay. Isolated dry game sound, no voices, no music.'),
    ('hull-impact',0.8,False,'One short heavy armored vehicle impact, low metallic chassis thud with a restrained crunchy rattle, tight and dry, immediate attack, no explosion, no debris shower, no voices, no music.'),
    ('countdown-tick',0.5,False,'One single short clean race countdown electronic beep, medium pitch, firm attack and very short decay, clear through engine noise, isolated interface sound. No sequence, no voice, no music.'),
    ('race-go',0.8,False,'One bright high electronic race start signal, a single clear sustained go beep with quick onset and clean short decay, noticeably higher than a countdown beep. Isolated game interface cue, no voices, no music.'),
    ('race-finish',2,False,'One concise futuristic race finish notification, a satisfying ascending electronic confirmation flourish ending in a bright resonant chime, dry polished game interface effect. No crowd, no speech, no background music.'),
]

if __name__ == '__main__':
    manifest=[]
    for name,duration,loop,prompt in ASSETS:
        manifest.append(dict(name=name,duration=duration,loop=loop,prompt=prompt))
    (ROOT/'docs/racing-audio-prompts.json').write_text(json.dumps(manifest,indent=2)+'\n')
    for a in manifest:
        output=ROOT/'artifacts/audio-jobs/racing'/f"{a['name']}.mp3"
        sidecar=output.with_suffix('.mp3.provenance.json')
        if output.exists() or sidecar.exists():
            if not (output.exists() and sidecar.exists()):
                raise SystemExit(f"Incomplete existing artifact: {a['name']}. Inspect before any paid retry.")
            saved=json.loads(sidecar.read_text())
            if saved['request']['text']!=a['prompt']:
                raise SystemExit('Existing prompt mismatch; refusing replacement')
            print('Keeping existing:',a['name'],flush=True)
            continue
        print('Generating:',a['name'],flush=True)
        args=[sys.executable,str(ROOT/'.agents/skills/elevenlabs/scripts/elevenlabs.py'),'generate','--repo',str(ROOT),
              '--prompt',a['prompt'],'--duration',str(a['duration']),'--output',str(output),'--prompt-influence','0.45']
        if a['loop']:args.append('--loop')
        subprocess.run(args,check=True)
