# Physical surfaces, compounds and impact observations

Use selected engine `docs/physical-surfaces.md` and its validation record, then
verify signatures in physical_surface.hpp, collider_component.hpp, physics.hpp,
vehicle_component.hpp, terrain.hpp and terrain_paging.hpp. These Stage 2 APIs may
be newer than a consumer engine pin; updating this skill does not update a gitlink.

Publish validated `shared_ptr<const PhysicalSurface>` resources and never mutate
retained non-const aliases. IDs are descriptive ASCII 1..128 characters; sharing
uses resource identity, not a global ID registry. Defaults are friction=1,
restitution=0, tyreGrip=1, rollingResistance=0. Validate finite ranges 0..10 for
friction/grip and 0..1 for restitution/rolling. Visual materials do not affect grip.

ColliderComponent::setSurface assigns fallback. CollisionShape::withSurfaces adds
a shape fallback or exact source-triangle map; null entries inherit. CompoundChild
has shared shape, unit-scale local Transform and optional surface override. Use
CollisionShape::compound(span), bounded 1..32 children per node, 8 child levels,
1024 expanded descendants, 100000 triangles. Dynamic/sensor/query compounds need
convex leaves; static bodies may include meshes. COM/inertia derive from geometry,
then authored COM offset/inertia settings apply. Shape/map replacement rebuilds
bodies, joints and vehicle constraints; collider fallback updates invalidate cached
contacts and wake dynamic scene bodies without replacing geometry.

Surface precedence: explicit triangle, child override instead of that child's own
shape default, descendant defaults/overrides, ancestor default, collider fallback,
neutral. Rigid friction is sqrt(body1*surface1*body2*surface2). Restitution is max
of body/surface values. Tyres use sqrt(wheelCurve*targetBody*targetSurfaceFriction)
* targetSurfaceTyreGrip on both axes. Wheel telemetry has surface and combined
longitudinal/lateral coefficients. Rolling resistance is an opt-in approximation:
previous grounded wheel coefficients/4 apply capped tangent COM drag mass*g*c,
not a wheel-load or tyre-deformation model.

PhysicsSurfaceHit on queries/wheels and both collision sides retains the immutable
surface, source child path and triangle (UINT32_MAX if absent). Topology edits
invalidate source indices. Resolve scene handles before use. Collision Begin/Stay
relativeVelocity is pre-solver velocity(second)-velocity(first) at the first manifold
point (world m/s), closingSpeed=max(0,-dot(relativeVelocity,normal)). Normal points
first to second; impactValid is false for End. Existing post-solver guarded delivery
is bounded at8192 events/step with droppedEvents overflow. These values are not
force, impulse, damage or one event per crash. Applications own cooldowns, sounds,
consequences and callback reconstruction after restore.

TerrainData and TerrainPage have physicalSurface and an optional exact row-major
cellSurfaces map, both triangles per cell. TerrainAsset::withPhysicalSurfaces returns
an immutable edit sharing visual geometry; full and streamed collision update.
WETP3 writes an optional bounded surface palette (256 resources/64KiB;256KiB/page).
Neutral pages remain WETP2; readers support1..3. Page worker hydration, halo agreement,
source/dirty budgets and writeback preserve/validate physical maps. Page sculpt/paint
preserves mappings; physical page painting is not delivered. Physical maps count
toward source budgets, including validation before edit/residency publication.

EditorSession::setCollider and setTerrainPhysicalSurfaces provide undo/redo and
invalid-state rollback. Browser Details physical panel authors surface assets,
bindings/triangle/cell ranges and compound child poses/add/remove/replace with
existing library shapes. Edited copies affect a chosen binding; shared resources
are immutable. Complete editable terrain only; page children remain protected.
Play disables authoring and isolates source. Vehicle panel reports surface/grip.

Current scene/application/effect envelope writer v31 deduplicates compounds,
physical resources/maps and catalogs. v24 introduced vehicle fields. Old1..24 files
retain neutral surfaces; old readers reject25. Keep host/main/worker artifacts
matched. Contact observations, commands, callbacks and solver caches are transient;
archives are not exact solver snapshots. Assets/subtree duplicates/Play use codecs.

Build with repository-local SDK pinned by .emscripten-version (6.0.11 here).
Native: physical_surface_tests and CTest physical_surface, plus physics, vehicle,
terrain, terrain_paging, terrain_page_edit, editor*, scene_archive, persistence,
prefabs. Wasm: build physical_surface_wasm_tests and run .js through Node.
Browser: publish web_assets, serve actual dist, set NODE_PATH to Playwright/pngjs
and DEMO_URL, run node scripts/physical-surface-smoke.cjs at DPR1 and DPR2 with
separate EVIDENCE_DIR. It forces WebGPU/WebGL2 and checks braking/turning, wheel
metadata, impacts, compound and terrain queries, editor history/invalid edits,
duplication, Play, file/IndexedDB restore and resize/recovery. Regress vehicle and
terrain paging/edit browser suites on both backends. Record browser/build/hash,
viewport/DPR, telemetry, screenshots and errors. Desktop checks do not establish
physical mobile, sustained fleet performance or human handling acceptance.

V25 introduced the physical fields; v26 adds authored camera composition. See [Stage 3](driving-controls.md).
