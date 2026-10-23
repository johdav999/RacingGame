"""Prepare retained ElevenLabs sources as bounded, level-matched mono WAVs."""
import array
import hashlib
import json
import math
import shutil
import subprocess
import wave
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
RATE=24000
out=ROOT/'assets/runtime/audio/racing'
out.mkdir(parents=True,exist_ok=True)
report=[]
for spec in json.loads((ROOT/'docs/racing-audio-prompts.json').read_text()):
    name=spec['name'];source=ROOT/'artifacts/audio-jobs/racing'/f'{name}.mp3'
    provenance=json.loads(source.with_suffix('.mp3.provenance.json').read_text())
    assert hashlib.sha256(source.read_bytes()).hexdigest()==provenance['sha256']
    decoded=subprocess.check_output(['ffmpeg','-v','error','-i',str(source),'-f','f32le','-ac','1','-ar',str(RATE),'-'])
    samples=array.array('f',decoded)
    assert samples and all(math.isfinite(v) for v in samples)
    peak=max(map(abs,samples));assert peak>.01
    source_duration=len(samples)/RATE
    if spec['loop']:
        n=int(RATE*.12);data=list(samples[n:]);start=len(data)-n
        for i in range(n):
            t=i/(n-1);data[start+i]=samples[-n+i]*(1-t)+samples[i]*t
    else:
        threshold=peak*.025
        first=next(i for i,v in enumerate(samples) if abs(v)>threshold)
        last=len(samples)-next(i for i,v in enumerate(reversed(samples)) if abs(v)>threshold)
        data=list(samples[max(0,first-120):min(len(samples),last+1200)])
        n=min(120,len(data)//4)
        for i in range(n):data[i]*=i/n;data[-i-1]*=i/n
    rms=math.sqrt(sum(v*v for v in data)/len(data));peak=max(map(abs,data))
    gain=min((.11 if spec['loop'] else .18)/max(rms,1e-8),.72/peak)
    pcm=array.array('h',(round(max(-1,min(1,v*gain))*32767) for v in data))
    target=out/f'{name}.wav'
    with wave.open(str(target),'wb') as f:
        f.setnchannels(1);f.setsampwidth(2);f.setframerate(RATE);f.writeframes(pcm.tobytes())
    entry=dict(name=name,file=target.name,loop=spec['loop'],sha256=hashlib.sha256(target.read_bytes()).hexdigest(),
        sample_rate=RATE,channels=1,frames=len(data),seconds=len(data)/RATE,peak=peak*gain,rms=rms*gain,
        source_seconds=source_duration,source_sha256=provenance['sha256'],
        loop_boundary_delta=abs(pcm[-1]-pcm[0])/32767 if spec['loop'] else None,
        preparation='Mono 24kHz PCM16, level matched with peak headroom; 120ms circular loop seam or trimmed one-shot with 5ms edge fades',
        verification='Decoded finite PCM, peak/RMS, duration and loop-boundary analysis; subjective listening not implied')
    provenance['prepared']=entry
    (out/f'{name}.wav.provenance.json').write_text(json.dumps(provenance,indent=2)+'\n')
    report.append(entry)
(out/'manifest.json').write_text(json.dumps(dict(version=1,sounds=report),indent=2)+'\n')
header='#pragma once\n#include <array>\nnamespace racing::sound {\nstruct Asset {const char* name;const char* hash;};\ninline constexpr std::array<Asset,13> assets{{\n'
for a in report:header+='    {"'+a['name']+'","'+a['sha256']+'"},\n'
header+='}};\n}\n'
(ROOT/'src/race_audio_contract.hpp').write_text(header)
print(json.dumps([dict(name=a['name'],seconds=round(a['seconds'],3),peak=round(a['peak'],3),rms=round(a['rms'],3),seam=a['loop_boundary_delta']) for a in report],indent=2))
