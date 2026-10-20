# Animation libraries and visual state machines

Verify the selected engine checkout contains `animation_library.hpp` and
`editor_animation.hpp`; development documentation does not advance a game pin.
Read that checkout's `docs/animation-authoring.md`, `docs/animation-control.md`
and `docs/animation-authoring-validation.md` before integrating.

`engine_core` supplies shared immutable `AnimationLibrary` resources:
`fromSkeleton(skeleton, optionalController)`, `create(AnimationLibraryData)`,
`append(sourceSkeleton)`, `clipIndex(stableId)`, `validateTarget(skeleton)` and
`apply(SkeletalMeshComponent&)`. Clip slots are append-only; display name/folder/tag
edits do not change stable IDs or runtime indices. `append` validates unique joint
names (one unnamed synthetic root allowed), matching parents and local bind matrices
within 1e-4, and remaps samples into the destination order. Target skin bindings
remain intact. Different bind poses need offline retargeting. The browser imports
model FBX/GLB files through existing decoders, not arbitrary animation-only files.

The library includes the existing indexed `AnimationControllerDefinition`, graph
positions, per-clip loop/speed defaults and optional root-motion joint. State loop/
speed are explicit; clip defaults apply when selecting a clip for a new assignment.
Gameplay calls `mesh.animation().setParameter("Attack", true)` for a Boolean, or
`trigger("Attack")` for a one-shot Trigger. Boolean persists until explicitly cleared;
Trigger latches until a winning transition consumes it. Number parameters can drive
two-clip blends. First eligible ordered edge wins; all its conditions must match.
Optional normalized exit 1 gates completion at the next variable-tick evaluation.
No additive/layered/hierarchical/any-state animation graphs are introduced.

The player owns clocks, poses, typed inputs, markers and motion output independently
per character. Call library apply on the idle scene owner; guarded traversal rejects.
Consume marker/root-motion output explicitly and keep damage, sound and collision
rules in the game. `lastTransition()` / `transitionCount()` are transient diagnostics,
reset on controller replace/detach, not a durable event stream or archive state.

`engine_editor` supplies independent `AnimationLibraryEditor` history (default 32,
range 1..128, 128 MiB unique retained sample budget including the saved version).
`EditorSession::selectedSkeletalMesh` and `setAnimationLibrary` provide guarded,
undoable staged animation-state exchange preserving saved IDs, node handles and
previous playback. Mesh/skeleton/player instances remain at their original addresses.
Custom subtrees unsupported by the existing clone codec reject before publication.

Details -> Animation library & state machine opens clip search/folders/tags,
marker editing, root selection, draggable states, clip/loop/speed/blend settings,
typed parameters, transition conditions/completion/crossfade/priority and independent
undo/redo. Preview uses the existing isolated Editor Play scene and live parameter
controls. Source scene/history/defaults remain unchanged; emitted motion is drained
without moving the actor. Document unbinding invalidates the target and file-read
generation. Close/Stop releases preview. Attack example is application authoring
policy and uses the first two clips; assign actual character actions afterward.

`AnimationLibraryArchive::save/load` stores `.weanim` v1, wrapping bounded metadata
and existing checksummed scene codecs. Save it separately for organization/layout.
Applied scene archives retain clips/controller/runtime, not library folders/tags/layout.
Scene reopen reconstructs a basic authoring library; open `.weanim` to recover metadata.
No library autosave, trimming, clip deletion or automatic rig retargeting is delivered.

Limits: 64 clips, 256 joints, 1024 bindings, 32 MiB sampled poses; 256 states,
64 parameters, 1024 edges, 64 conditions/edge; text <=256 bytes/no controls;
speed (0,100], fades <=3600 seconds, normalized exit <=10000, positions +/-10000.
Files <=64 MiB, decoded allocations <=128 MiB, metadata <=512 KiB. Invalid edits,
rig mismatches and malformed files preserve previous authoring/scene state.

Validation: build native `animation_library_tests`, `animation_control_tests`,
`animation_tests`, `root_motion_tests`, `editor_content_tests`, `scene_archive_tests`;
run matching CTest names. Use the repository-local SDK pinned by `.emscripten-version`
for the browser build, then serve that exact site. Run
`scripts/editor-animation-smoke.cjs` with NODE_PATH/BROWSER_EXECUTABLE/DEMO_URL,
including WebGPU/WebGL2 and DEVICE_SCALE_FACTOR=2. It exercises Boolean Attack,
consumed triggers, visible graph/clip preview, exact transition diagnostics,
drag cancellation/history, source-byte isolation, compatible import and library/
scene persistence. Desktop Chromium evidence is not physical Android/iOS acceptance.
