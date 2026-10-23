# Racing audio

Implemented 2026-10-22 using the repository ElevenLabs skill and the pinned
WebEngine `AudioSystem` / `BrowserAudioBackend`. Works on Kairox and Desert Solar.

## Controls and behavior

Enter a race or click **SOUND → ENABLE SOUND** to activate browser audio.
The sound panel has saved master, engine, effects and race-cue volumes plus mute.
Opening sound settings during a race pauses it. Resume explicitly to keep racing.
Browser autoplay restrictions are respected; no sound starts without a gesture.

Each chassis has its own generated loop. Car pitch follows actual drivetrain
RPM, including gear changes; hover pitch follows body speed and throttle.
Gain and pitch are smoothed without restarting the source. The garage previews
only the selected chassis. Five opponents have spatial HRTF voices following
their actual world positions, with distance attenuation and the chase-camera
listener. Player sounds stay centered for readability.

Boost follows actual reactor boost state. Grounded wheeled lateral motion starts
the tire loop; hovercraft do not squeal tires. Confirmed shots, shield activations,
shield blocks, received hits and sufficiently strong side collisions trigger
effects. Collision sounds have a 300ms cooldown; resting ground contact is silent.
Countdown numbers, race start and player finish have distinct cues.

Pause and hidden-page boundaries suspend the audio device clock. Resume requires
a gesture. Transient sounds are cleared at suspension; observed event counters
are consumed while inactive so old shots cannot replay on resume. Restart,
chassis selection and scene changes clear obsolete voices. Finish stops driving
loops and plays one result cue. Audio never changes simulation or rewards.

## Assets and provenance

Thirteen successful ElevenLabs `eleven_text_to_sound_v2` requests generated:

| Type | Sounds |
| --- | --- |
| Engine loops | Vektor, Skimmer, Bastion, Vector |
| Driving loops | Tire slide, reactor boost |
| Combat/impact | Laser shot, shield activation, shield impact, hull impact |
| Race cues | Countdown tick, go signal, finish chime |

The provider returned a total **388** in its `character-cost` headers; this is
the provider's usage unit, not a currency amount. No paid retries were made.
Prompts and settings are in `racing-audio-prompts.json`. Raw MP3s and original
sidecars remain in ignored `artifacts/audio-jobs/racing/`.

Runtime files and sanitized provenance live in `assets/runtime/audio/racing/`.
The 13 mono 24kHz PCM16 WAVs total **1,712,060 bytes**. Preparation level-matches
loops to RMS 0.11 where peak headroom allows, caps peaks at 0.72, crossfades loop
boundaries over 120ms, and trims one-shot silence with short edge fades. Runtime
imports verify SHA256 before browser decoding and native SoundAsset publication.
Provenance records source/prepared hashes, processing and actual measurements.
Account rights remain unverified in those provenance files; no release-rights
claim is made by this integration.

## Ownership and budgets

`src/race_audio.*` is a portable game-owned mixer/controller using existing engine
audio APIs. `StarterScene::audioFrame()` exposes an immutable presentation snapshot
of real vehicles, combat counters, collisions and race state. The browser host owns
the mixer and supplies pause/suspension policy. `web/race-audio.js` owns asset fetch,
gesture activation, saved settings and status UI; `audio-system.js` is copied from
the selected engine during packaging. No adjacent engine edits or pin change.

The budget is 32 total voices, including six engine loops, up to six boost loops,
one player tire loop and at most 12 retained one-shots. Overloaded effects evict
the oldest one-shot; missed events are never queued indefinitely. Sounds share
immutable PCM. Failed downloads show a retry action while the game remains usable.
Volume settings use a separate storage key and never rewrite career progression.

## Validation

- `racing_audio` native test: activation, import rejection, loop identity, RPM
  response, spatial positions, voice cap, event deduplication, all mixer buses,
  mute, pause/resume, recovery, finish, garage/chassis changes and real scene
  telemetry. Portable Windows core evidence; not a native desktop audio-device test.
- Native `racing_smoke`, laser, weapon and shield regressions passed after the
  audio/collision hooks were added.
- `scripts/race-audio-smoke.cjs`: real Web Audio output measurements on forced
  WebGPU and WebGL2, 13 loaded/hash-checked assets, no pre-gesture playback,
  audible/nonzero mix, true mute silence, four chassis with real keyboard driving,
  pitch/boost/fire/shield cues, positional opponents, countdown/go, pause clock,
  recovery, garage cleanup, saved settings across tracks and narrow layout.
- `scripts/race-audio-lifecycle.cjs`: failed-download retry and real grounded
  tire-slide checks passed; captured the actual gameplay mix. This automation host
  does not report a hidden document after minimizing Chrome, so a real visibility
  transition remains unverified. The main matrix verifies explicit pause/resume
  and the page-hide suspension handler; that is not a substitute for real tab QA.

Reports, screenshots and the captured mix are in `build/evidence/race-audio/`.
Audio QA establishes decodability, finite samples, measured headroom, loop-boundary
processing and real device output. It does not substitute for human listening
approval of timbre or generated content, or testing on every physical audio device.

Reproduce asset preparation with `python scripts/prepare-race-audio.py` when the
retained raw sources are available. Builds and tests require no provider key or
paid calls. `generate-race-audio.py` is an explicit production tool, not a build step.
