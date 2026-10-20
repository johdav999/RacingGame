# Browser editor workspace

Delivered in WebEngine on 2026-10-16. Verify the selected checkout and consuming
game's engine pin before assuming this editor host is present in that game.

`web/editor-layout.js` / `.css` compose Scene Outline and Quick Add on the left,
Scene in the center, Details on the right and Assets below Scene. Existing DOM
controls and the actual canvas move without recreation. Layout owns no scene or
GPU state. The existing editor/content/structure adapters remain authoritative.

Drag titles to float within the browser workspace or drop onto Left, Center,
Right or Bottom targets. Title docking menus provide a keyboard alternative.
Dock dividers resize columns/bottom height; panel bottom grips resize stack shares;
floating corner grips resize width/height. Focus grips and use arrows. Escape,
pointer cancellation, blur and visibility loss roll back unfinished gestures.
Window restores hidden panels or resets the layout. Scene cannot be hidden.
Validated v1 localStorage under `webengine.editor.layout.v1` is separate from scene
files and history. Invalid layouts fall back safely; storage failure retains this
tab's working layout. Floating means in-page, not native OS windows. At <=800 CSS
pixels panels stack with Scene first and docking/grips are hidden.

Quick Add Cube/Sphere/Plane click-then-place or drag/drop invokes browser export
`engine_editor_primitive(kind,x,y)` (kinds 0/1/2, canvas backing pixel coordinates).
It composes existing Mesh factories and `EditorSession::placeMesh`, preserving Y=0
placement, snapping, one-command history, dirty state and ordinary scene archives.
Light/Camera/Mesh/Collider/Body/Interaction shortcuts attach to the selected actor
using existing component commands; Mesh needs the selected catalog asset. Preserve
topology and protected-node constraints. Play/busy/transform gates still apply.
Do not add direct JS scene mutations or serialized layout data to scene archives.

Build with README's local pinned Emscripten 6.0.11. Stage matching host scripts,
CSS, index, Wasm and worker artifacts. Run `scripts/editor-layout-smoke.cjs` using
Playwright, `DEMO_URL`, `BROWSER_EXECUTABLE`, `EVIDENCE_DIR`, optionally
`DEVICE_SCALE_FACTOR=2`. Run editor-smoke, editor-content-smoke and
editor-structure-smoke as regressions. Run browser suites serially if the local
HTTP server cannot accept their concurrent request volume. Check real pixels,
canvas identity after docking, history/selection after Play, save/load, storage
failure, layout reset and narrow-screen overflow. See docs/scene-editor.md and
docs/scene-editor-layout-validation.md for delivered scope and evidence.
Desktop/browser emulation does not establish physical mobile acceptance.
Rotation-ring hit tests use the closest projected centerline when thick hit strokes
overlap in resized viewports. Keep actual pointer-ring regression coverage.

## Camera navigation and outline framing

`web/editor-camera.js` captures right/middle mouse on the canvas/gizmo surface.
RMB holds a temporary opaque EditorFly context: WASD use Move, Q/E use host action
0xED01 and Shift uses 0xED02. Mouse emits Look; middle drag emits Pan. W/E/R retain
tool shortcuts outside navigation. Cancel on Escape, blur, hidden page, field
focus, resize, busy/Play transitions, exit and document binding; release capture,
held sources and temporary mappings. No pointer lock is used. Dragging is bounded
by the browser screen; it is not an unlimited OS cursor-lock mode.

Portable `OrbitController::look(dx,dy)` changes yaw/pitch with fixed eye and
rebuilds the orbit target; `translate(Vec3)` uses camera-local right/up/forward
(positive Z is forward); `focus(Vec3,float)` takes parent-space target and distance.
`orbit(dx,dy,minElevation=.05,maxElevation=1.45)` and
`zoom(delta,minDistance=3,maxDistance=28)` retain legacy defaults. Look clamps pitch
to +/-1.55; the editor uses these orbit limits and zoom distance .05..1e6.
Reject non-finite new navigation before publication. Saved orbit restore still
preserves the independently archived camera transform exactly.

The browser host borrows the authoring controller alongside its scene/camera and
clears pointers before replacement. It consumes the application input clock while
paused; normalized movement is 5 parent-space units/sec (Shift 20). Play owns and
updates its own controller, restoring the retained source view on Stop.
`engine_editor_focus(savedId)` resolves the authoritative node, computes active
subtree mesh bounds including skeletal pose bounds, and fits the current aspect/
lens with margin. Meshless/inactive nodes use their position. It retains viewing
direction, sets the orbit pivot, adjusts orthographic height and expands far clip
when needed. Double-click/F invoke it outside busy/Play/transform operations.
Camera navigation does not modify object undo/dirty revision, but scene saves
include the authoring camera/orbit fields as before.

Build/run native input, camera and camera_controller tests, then
`scripts/editor-camera-smoke.cjs` with DEMO_URL and BROWSER_EXECUTABLE. Repeat with
DEVICE_SCALE_FACTOR=2 and separate EVIDENCE_DIR. Check eye-fixed look, flight,
field/shortcut isolation, cancellation, actual visible framing, new orbit pivot,
Play/Stop authoring bytes and floating Scene interaction. Run the existing four
editor suites serially as regression checks. See docs/scene-editor.md and
docs/scene-editor-camera-validation.md for exact evidence and scope.

## Terrain editor

`src/editor_terrain.cpp` extends the optional portable EditorSession, composing
TerrainAsset::edit/applyTerrainEdit rather than duplicating terrain math.
`createTerrain(TerrainData,Vec3 sceneLocalOrigin={},bool collision=false)` stages a
complete detached actor and uses ordinary creation history. `terrainActor()`
resolves a selected chunk/actor; `terrainReadOnlyReason` checks ownership.
`beginTerrainStroke`, `stampTerrain(TerrainBrush)` (source-local coordinates),
`commitTerrainStroke` and existing `cancel` provide one command per gesture.
The session retains initial/final immutable sources and unioned affected chunks;
previews reconcile source/visual/collision without ticks but do not advance dirty
revision. Commit advances once. Existing history handles undo/redo, saved-state
checkpoints and branches. Parent/pose/fingerprint guards reject external writes.
Only complete eager sources with identity direct chunks and static collision are
editable. Source-only streams, partial/paged instances remain stream-owned and
reject editor strokes. History retains source resources within the session limit.

`web/editor-terrain.js` supplies a Terrain section moved into Details by the dock
layout. Quick Add Terrain uses 65² samples over 64² metres with collision and four
colored layers. Creation options allow 33/65/129² samples, 1–512m square extent,
optional static collision and an imported texture heightmap. Decoded 8-bit red maps
0..heightRange, nearest sampled without flip/gamma into the chosen grid; existing
texture import limits apply. No new file format or importer was added.
Raise/Lower use signed height deltas; Flatten/Smooth/Paint blend by strength
.01..1. Radius is .1..128 source-local metres. Stamps follow movement at .2 radius
or repeat after 100ms. A source-projected ring previews footprint. Capture left
pointer only when brushing; retain RMB/WASD, middle pan and Alt orbit. Escape,
blur, hidden page, lost capture, busy/Play and target/document changes cancel the
whole stroke. W/E/R return to object tools. Painting requires authored layers and
retains geometry/collision. Layer texture assignment is now available; see
[layer texture integration](editor-terrain-textures.md).

The terrain demo suspends its stream/input updates in editor mode, then adopts
completed source versions and clears its own stale history on return. Source
rebuild is synchronous without a worker budget; large imported sources cost more
than creation presets. Scene save/autosave/Play use existing terrain codecs.
Run native editor_terrain, editor, editor_content, editor_structure and terrain,
then scripts/editor-terrain-smoke.cjs on WebGPU/WebGL2 at DPR1/DPR2. Run all five
existing editor scripts as regressions, plus terrain-edit-smoke for the demo.
See docs/scene-editor.md and docs/scene-editor-terrain-validation.md.
