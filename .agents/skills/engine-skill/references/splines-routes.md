# Splines and procedural routes

Verify the selected checkout's `docs/splines-routes.md` and its validation record.
These development APIs may be absent from the game dependency pin.

Portable `SplineAsset::create(SplineData)` copies immutable Hermite source knots:
position, tangent (metres per segment), bank radians. Open2..256 / closed3..256,
no repeated closing point; finite coordinates +/-100000, tangent magnitude
.0001..10000, bank +/-pi. C1 position/tangent continuity, C0 linear bank, not C2.
Stationary derivatives (including analytic between-sample zeros), coincident
adjacent knots and unresolved sampled reversals reject. Source length .0001..1e6.
`sample(u)` uses segment units; `atDistance(d)` uses approximate arc metres. Open
clamps and closed wraps (including negatives). Chord density defaults64, range
8..128; choose density/subdivision for application error tolerances. `nearest`
scans the complete table with budget1..32768(default32768), then16 local ternary
refinements. Budget exhaustion returns empty, never a partial best. It is an
approximate query, not an exact polynomial/global intersection solver. Sampling
rejects numerically unresolved derivatives below1e-6; use source-local coordinates.

`SweepProfile::create(SweepProfileData)` stores2..32 open or3..32 closed across/up
pairs (+/-1000), immutable shared Material and optional PhysicalSurface. A flat
left-to-right open edge faces up; clockwise perimeter faces outward. Hard profile
edges and no end caps. `RouteAsset::create(RouteData)` references spline/profile;
spacing1m(.01..100), textureMeters4(.01..10000), collisionfalse, maxRings1025
(2..4097). Parallel transport, deterministic vertical seed, closed holonomy
correction and bank produce ordinary Mesh geometry with tangents/bounds. UV X is
profile distance and Y is curve distance divided by texture length. Closed seams
duplicate an exact closing frame with continuing UV distance, not forced integer
texture phase. Analytic profile-edge tangents agree across the distance-UV seam.
Reject local folds/reversals, >262144 vertices/triangles or conservative64MiB
rebuild estimate. Builds are synchronous complete candidates, no camera LOD,
automatic chunking, workers, lane semantics or junction solving.

`makeRouteActor(name,asset)` / `RouteComponent(name,asset)` require an own top-level
actor, identity-local RouteComponent and unit world scale when colliding. Protected
static body/collider children are managed; never edit them or the generated mesh
independently. `configure` requires an idle hierarchy; synchronize before physics
queries. Physics allows protected static children under one identity-local direct
component container; moving bodies still attach directly to root actors. Body
rebuilds invalidate contacts/joints according to existing contracts. Mesh/collision
use identical source samples and stay independent of camera/culling/interpolation.

Portable EditorSession `createRoute`, `selectedRoute`, `setRoute` provide creation
and exact immutable-resource grouped Undo/Redo with synchronization rollback.
Details -> Splines and swept routes supports point/tangent/bank/profile authoring,
world-grid snap, physical-surface selection, UV/spacing/budgets, insertion/removal,
duplication and geometry/collision previews. Drafts remain local until Apply and
cancel on binding/selection/Play changes. Explicit terrain point sampling bakes
all point positions +.02m over a20km vertical span or rejects the whole operation; tangents stay authored.
No retained terrain dependency/version subscription; resample deliberately after
terrain changes. This does not conform every profile vertex between knots.

Archive v27 introduced shared RouteAsset->SplineAsset/SweepProfile
and material/surface refs, not derived geometry payloads. Source counts are bounded
before allocation; derived CPU/cooking estimates charge ArchiveLimits before each
unique route rebuild. Required child IDs/settings persist and validate. Duplicates
and EditorPlaySession clones share immutable assets/meshes; file loads rebuild once
per route identity. Orphan spline/profile libraries are not introduced; retain
component refs or explicit application persistence. Independently archived ordinary
MeshComponents using generated geometry are separate baked meshes on reload.
Prefab registry factory RouteComponent accepts shared_ptr<const RouteAsset>.

Example `?editor=1&scene=route`: elevated curved/banked road and car placement are
application content in src/route_example.hpp/main host, outside reusable APIs.
Validate native spline_route/full CTest, explicit Node spline_route_wasm_tests,
and scripts/spline-route-smoke.cjs forced WebGPU/WebGL2 atDPR1 and2. Check source
sharing, curve/UV/frame/collision math, malformed files, grouped edits, terrain
bake, source hash after Play, scene/IndexedDB reload, real driving and recovery.
Keep archive Uint8Arrays inside browser page during tests instead of transporting
large byte arrays through Playwright. Include physics, vehicle/editor/archive
regressions. Inspect solid/collision/closed-route screenshots and record exact
artifacts/results. Desktop/emulation does not establish physical mobile or
sustained long-route performance. Generic editor history is count bounded; choose
smaller history/rebuild limits for large resources. No game pin is advanced.

Current scene/application/effect writer v31 reads v1..31 and retains v27 route resources.
