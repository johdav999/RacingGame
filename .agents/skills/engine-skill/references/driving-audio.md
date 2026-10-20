# Playback rate and driving audio

Verify `docs/audio.md`, `docs/driving-audio.md` and their validation records in the
selected engine checkout. Development APIs can be absent from a game dependency pin.

Portable `AudioPlayOptions::playbackRate=1` and
`AudioSystem::setVoicePlaybackRate(voice,float)` support finite [.125,8].
`voicePlaybackRate` returns the last accepted value, or 0 for unknown handles.
Invalid rates throw without publication; foreign/stale/backend-rejected controls
return false. Backends implement `bool playbackRate(uint32_t,float) noexcept`:
false preserves previous rate/source; success never restarts the source. The
browser schedules AudioParam changes at device currentTime, effective next audio
quantum. Pending/suspended AudioParam reads can show old rendered values; validate
scheduled options and resumed samples. Offsets count source seconds; constant-rate
one-shots last (duration-offset)/rate active seconds, loops wrap in source time.
Mute does not pause; suspend freezes device time and requires fresh activation.
Rate does not change simulation/fade time. No reverse playback, Doppler or separate
time stretching/pitch shift. Existing backends must implement the new method.

The browser-only `src/driving_audio_demo.cpp` owns generated loops, RPM/reference
rate, absolute-throttle idle/load crossfade, contact slip/gravel loops and
surface/closing-speed impacts. No engine/vehicle audio policy is implied. Its
private proxy scene and handle-scoped target do not retain source scenes; the host
prepares a scene-owned collision forwarding wrapper preserving existing callbacks.
Re-enabling after suspension preserves the four driving sources/cursors.
Stop/unbind/scene replacement/shutdown release/disarm voices, and pause/target
loss release them. Sample selection is illustrative (exact gravel ID vs hard
fallback), bounded to16 voices and .15-second impact cooldown.

Editor Details -> Driving audio and rate preview provides referenceRPM500..8000
(default2000), slipGain0..2(default.3), volume0..1(default.4), previewRate.125..8
(default1), isolated shared-sound preview and separate64-entry undo/redo. Playback
requires a gesture; authoring is disabled during Play. Version1 .wedriveaudio JSON
contains exactly version/baseRPM/slipGain/volume/previewRate; files <=4KiB validate
atomically. localStorage key webengine.drivingAudio.v1 keeps host tuning, never
enabled/device/voice state. Transfer tuning beside scene files; it is not scene-
specific. Stage4 added no audio codec; current scene archives arev27 after Stage5; no built-in SoundAsset/emitter/listener codec
was added. Reconstruct synthesized assets/buses/mappings from application tuning.
Custom persistent components still require explicit codecs and resource resolution.

Validate native CTest audio/full suite, `scripts/driving-audio-smoke.cjs` forced
WebGPU/WebGL2 atDPR1/2, and `scripts/audio-system-smoke.cjs`. Set NODE_PATH to a
Playwright installation and DEMO_URL to the freshly built site; set DPR=2 for the
second driving run. Use pinned .emscripten-version/repository SDK and matched
web_assets output. Inspect screenshots and results.json: sampled frequency,
source identity, offset/completion, mute/suspend, real vehicle slip/contact/impacts,
tuning history/files/reload, editor source hash/Play/Stop and scene cleanup. Do not
substitute mocks for browser audio nodes or claim physical speakers/mobile/listening
acceptance from analyser samples. Other browser input/vehicle smoke regressions
remain relevant when host lifecycle integration changes.
