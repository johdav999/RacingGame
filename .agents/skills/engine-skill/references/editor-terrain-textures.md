# Terrain layer texture assignment

The development browser editor provides four selectors under Details → Terrain →
Paint layer textures. Import images through Assets first. Select a terrain actor
or chunk, assign each layer's image, or choose None (layer color) to clear. The
selectors use the existing native texture catalog and refresh after imports,
selection/history and document changes. They are gated during busy/Play/active
gestures and for read-only or non-layered sources. Floating Details and compact
layouts retain the same controls. Textures multiply existing color and use the
existing UV tiling. These selectors assign albedos only. Runtime layer normal maps
use MaterialLayer::normalHeightTexture; configure those through the engine API. See
[terrain surface integration](feature-integration.md#terrain-surface-relief-development-engine-2026-10-17).

Portable TerrainAsset::withLayers(const MaterialLayers&) returns a validated
TerrainEdit, sharing all geometry/LOD chains and retaining source heights and patch
metadata. Appearance replacements use paintChanged=true and heightsChanged=false;
unchanged complete definitions return unchanged source pointers. Validation of the
whole material precedes construction/publication. Source-only assets remain
source-only; a stream owner publishes through its existing apply path.

EditorSession::setTerrainLayerTexture(size_t layer, shared_ptr<const Texture>)
accepts 0..3 and null to clear. It rejects active gestures and uses the existing
complete eager-instance ownership checks. It replaces one source layer texture,
retaining paint weights, layer colors/roughness/UVs, material settings and the other
layers, and applies to every chunk using applyTerrainEdit. No geometry or collider
is rebuilt, and component handles/other instances sharing the old source remain
unchanged. The same TerrainChange history guards and saved identities support
undo/redo, no-op detection, dirty revision and replay. Fingerprints include layer
parameters and resource bindings. Do not replace only one chunk's material and
leave the source stale: later painting would overwrite that change.

The native browser command engine_editor_terrain_layer_texture(layer, index)
uses the current catalog index; -1 clears. Native snapshots report four texture
indices to JS. Existing scene/material/terrain codecs retain images and weights
through save/recovery/Play; no new file format is needed. Layer textures obey the
existing validated RGBA8 fallback and maximum 4096px per axis. Keep browser/device
details out of the portable APIs.

Build with local pinned Emscripten 6.0.11 and stage matching host/Wasm/worker files.
Run CTest editor_terrain, terrain, editor, editor_content and editor_structure.
Run scripts/editor-terrain-textures-smoke.cjs with DEMO_URL, BROWSER_EXECUTABLE,
EVIDENCE_DIR and DEVICE_SCALE_FACTOR=1/2, plus terrain/content/structure browser
regressions. Validate real pixels for assigning/clearing and repainting; undo and
scene reopen should restore exact scene pixels once UI focus outlines/gizmos are
excluded from comparisons. Verify source sharing, weights, geometry and collision
in native tests. Read docs/scene-editor.md and
docs/scene-editor-terrain-textures-validation.md for recorded scope/evidence.
Desktop/high-DPI browser checks are not physical mobile acceptance.
