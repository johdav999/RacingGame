# Spotlights (development engine, 2026-10-19)

Verify the selected checkout/game pin. LightType::Spot extends LightComponent in
engine_core; there is no browser/GPU type in the portable API. Ordinary scene
hierarchy mounts headlights/torches. Applications own controls, light switching,
vehicle rules and aggregate light budgets.

Configure direction (local emitted vector), color/intensity/range, visibilityMask,
innerAngle and outerAngle. Half angles are radians, default pi/9 and pi/6 (20/30
degrees); default range10, intensity1, white, direction(0,-1,0), mask1, shadows off.
Require finite nonnegative color/intensity, finite range>0 (Spot >=.001 m),
0<=inner<outer<pi/2 and cos(inner)-cos(outer)>=1e-6. Directional/Spot direction
must be nonzero; unused zero Point direction is retained for older scenes. World
pose must remain finite and emitted direction nonsingular. Local vectors transform
with hierarchy then normalize; range and half angles stay world units despite scale.

The law is clamp(1-distance/range,0,1)^2 times
smoothstep(cos(outer),cos(inner),dot(emittedDirection,-receiverToLight)). Existing
near-source division guards apply. It scales direct diffuse/specular and water
highlights, leaving ambient/IBL/emission/unlit unchanged. spotlightAttenuation is
the portable reference for finite nonnegative distance and cosine[-1,1]. This is
the existing finite-range law, not inverse-square photometry.

Both renderers report RenderCapabilities maxLights=8, spotLights=true and
spotLightShadows=true (also EngineQuality). The first eight active enabled masked
lights in scene traversal order share the budget across all types; zero intensity
occupies a slot. No ranking/clustered lists. Spot shadows are optional; Point shadows remain rejected.
Directional cascades and spots share the existing map budget. GL retains basic lighting.
No volumetric beams. See [lighting](lighting-post-processing.md) for optional shadow passes and HDR.

Use validate() for common/settings validation; validateSettings() is allocation-free
for warm collection. Keep private WGSL/CPU light layout and retained frame cache in
sync: 64-byte light payload and2336-byte WebGPU FrameUniform; GL adds cone uniforms.
Scene collection and renderer must not write simulation/light state.

Editor Components & properties -> Light exposes Spot, half angles in degrees,
direction/range/mask and appearance. Existing property rollback, undo/redo,
duplication and isolated Play apply. Spot accepts the existing shadow toggle.
The docked Spotlights panel controls pointer-transparent selected cone outlines;
spotlightOutline(light,segments=32) returns world inner/outer rings/rays/axis,
2*segments+9 line pairs, segments8..64, with ring ends on the range sphere.
No retained scene references or physics effects. Open headlight example opens a
separate page ?editor=1&scene=spotlight, preserving the current document.

Archive v30 appends angles and accepts Spot; current readers1..31 retain older layouts and
20/30-degree defaults. Scene/application saves, built-in clone/duplicate/Play and
PrefabLightSettings retain authored data. Staged loads reject invalid cones/types/
shadow combinations before publication. Render buffers/SVG remain transient;
publish matched host/worker/Wasm artifacts. Do not advance consumer gitlinks implicitly.

Build with pinned local Emscripten6.0.11 per README. Run native CTest spotlight,
render_frame, shadow, editor, prefab, scene_archive/persistence; explicit
spotlight_wasm_tests in Node. scripts/spotlight-smoke.cjs covers WebGPU/WebGL2 at
DEVICE_SCALE_FACTOR1/2, cone pixels/parent transforms/masks/eight-light budget,
MSAA, editor/history/invalid retention/Play, v30 persistence and GPU recovery.
Check scripts/shadow-smoke.cjs and water-smoke.cjs when touching light payloads.
Use NODE_PATH/BROWSER_EXECUTABLE/DEMO_URL/EVIDENCE_DIR as in build-validation.
Desktop/DPR checks do not establish physical mobile or sustained maximum performance.
Read selected-engine docs/spotlights.md and spotlights-validation.md for evidence.
