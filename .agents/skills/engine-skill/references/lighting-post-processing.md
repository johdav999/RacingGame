# Lighting, shadows and HDR composition

Verify these APIs in the selected engine checkout. A Paws gitlink is not advanced
by this documentation. Read docs/lighting-post-processing.md and its validation
file before changing render paths.

LightComponent Spot now accepts shadow.enabled=true on both browser backends.
Point shadows remain rejected. Existing editor/history/Play/prefab/archive paths
persist the flag; no archive version is added. ShadowSettings bias fields apply;
spot volumes use world pose, 2*outerAngle FOV, near=max(.00001,range*.001), far=range.

RenderQuality.directionalCascades=1 preserves fixed authored boxes; 2..4 fit
perspective/orthographic camera slices. shadowDistance defaults80 (finite >0),
cascadeLambda=.6 (0..1). Shared maxShadowMaps0..4 bounds total sun slices plus
spots in first-eight-light scene order. A directional light consumes up to its
requested slices from the remaining budget. Slices use stable XY and 10% blend;
ShadowSettings.halfDepth adds caster padding. Off-camera, masked, skinned/morphed,
mirrored and shadow-LOD geometry retain ordinary contracts. Core planShadows owns
no scene state. Depth arrays/commands are private; WebGL uses unit13. The private
WebGPU FrameUniform is2336 bytes with four96-byte map records. Keep shader/staging
layouts synchronized. No point cubemaps or volumetric beams.

RenderQuality.post is PostProcessing: enabled=false, exposure0 EV(-8..8),
bloomStrength.15(0..2), bloomThreshold1(0..32 scene-linear before exposure),
saturation1(0..2), contrast1(0..2). Full candidates validate before publication.
Enabled scene/reflection/water color is RGBA16F; mesh/sky/FX/underwater composition
stays linear. MSAA resolves once, then quarter-resolution bright extraction and
25-tap tent bloom plus exposure/Reinhard/saturation/contrast/sRGB; GUI follows.
Bloom can span composite view boundaries and grades Overlay meshes, never GUI.
Zero bloom skips extraction. No HDR display output, LUT, auto-exposure or filmic
curve. Renderer policy is transient, not scene archive preference.

RenderCapabilities.hdrPostProcessing is true after WebGPU acquisition; WebGL2
requires EXT_color_buffer_float and checks float4x separately, falling back to1x
when needed. Unsupported float devices keep requested post policy but render the
standard path. Browser EngineQuality reports postEnabled/effectivePost,
hdrPostProcessing and directionalCascades. Both report spotLightShadows=true.
WebGL remains basic-lighting/no-IBL/no-instancing/no-GPU-timing. Resize, disabling
post and recovery invalidate/release dependent resources without simulation edits.

Rendering quality -> Lighting & post-processing provides controls; Spot's existing
Light shadow toggle has undo/redo. Use pinned local Emscripten6.0.11 and README.
Explicit browser builds must target web_assets to stage matching generated site
and current host into dist; webengine alone does not publish. Native targets:
shadow_tests,spotlight_tests,render_quality_tests,render_frame_tests,scene_archive_tests.
Run scripts/lighting-post-smoke.cjs and lighting-composition-smoke.cjs on both
backends, plus shadow-smoke.cjs, spotlight-smoke.cjs and water-depth-smoke.cjs when
changing these paths. NODE_PATH/BROWSER_EXECUTABLE/DEMO_URL/EVIDENCE_DIR follow
references/build-validation.md. Post tests cover actual shadow/bloom/grade pixels,
invalid retention, disable restoration, MSAA, resize/recovery; composition covers
HDR water, underwater fog and forced missing float support. Report actual runs;
desktop/DPR evidence is not physical mobile or sustained performance acceptance.
