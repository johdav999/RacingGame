# Additional vehicle model generation

Requested scope, 2026-10-20: Bastion heavy car, Skimmer agile hovercraft,
and Vector fast hovercraft. The existing Cybercar is unchanged.

## Deliverables

| Asset | Local GLB | Triangles | Embedded maps | Status |
| --- | --- | ---: | --- | --- |
| Bastion | `artifacts/model-jobs/bastion/model.glb` | 45,241 | Color, normal, ORM; each 4096 x 4096 | Downloaded; four Blender views inspected |
| Vector | `artifacts/model-jobs/vector/model.glb` | 47,399 | Color, normal, ORM; each 4096 x 4096 | Downloaded; four Blender views inspected |
| Skimmer | `artifacts/model-jobs/skimmer/model.glb` | 47,314 | Color, normal, ORM; each 4096 x 4096 | Recovered and downloaded 2026-10-21; four Blender views inspected |

Each completed model has `model.glb.provenance.json`, `qa.json` and four
`qa-*.png` renders beside it. Original provider geometry and maps are retained.
These working outputs are ignored by Git, as required by `docs/assets.md`.
Prepared runtime copies are now integrated into gameplay; see
[vehicle integration](vehicle-integration.md). Raw outputs above retain the
provider's 4K maps and normalized source coordinates.

Built-in ImageGen created each isolated studio reference. Exact prompts,
reference PNGs, source-document references and SHA-256 values are retained in
`artifacts/model-input-images/{bastion,skimmer,vector}/` as `prompt-v1.txt`,
`reference-v1.png` and `provenance-v1.json`.

Generation used the repository Tripo defaults: v3.1 geometry, detailed geometry
and texture tiers, v3.5 textures, PBR on, 50,000-face target, triangle topology,
AI Complete off, Remove Lighting off. No segmentation or replacement generations
were purchased. Each of the three models reports 60 provider credits consumed.

## Validation boundary

All three completed GLBs import into Blender 3.5.1 and render with embedded maps.
Front three-quarter, opposite three-quarter, side and top views were inspected.
Bastion retains four wheels and its broad armored body; Vector retains four lift
pods, tapered hull and rear thrusters. Skimmer retains four squat lift pods,
central cockpit and rear fins. They are each one mesh object with one
material, without a rig or independently bound wheels/pods.

Provider coordinates are normalized, not meters: Bastion measures approximately
0.980 x 0.519 x 0.279 in Blender XYZ; Vector 0.985 x 0.622 x 0.209;
Skimmer 0.980 x 0.732 x 0.221.
The long axis is X, Z is up after Blender's glTF conversion, and the nose points
toward +X in the inspected renders. Before integration, normalize orientation
and scale, review their relatively wide footprints, prepare animated parts and
pivots where needed, optimize textures/LODs and validate WebGPU and WebGL2.
Static Blender QA does not establish game performance, collision fit or device
acceptance. Account entitlement and release rights remain unverified.

## Task recovery and helper compatibility

The installed helper accepts only task IDs beginning `task_`, but live Tripo
returned UUID task IDs. The initial Skimmer submission at
2026-10-20 19:19 UTC (21:19 Europe/Stockholm) was accepted. Tripo Account > Usage
showed the matching Image to Model task beginning `a92da776`. The original helper
rejected its ID before saving it. On 2026-10-21 the full ID was recovered using
the row's Copy task ID button and native browser paste into the visible task
search field. The browser virtual clipboard had returned empty data. The ID was
saved into the original job and resume successfully downloaded the existing
result. No replacement submission or additional generation charge was needed.

An isolated helper copy under `artifacts/model-jobs/vehicle-tools/` accepts both
UUID and `task_` IDs. It preserves the installed skill's generation settings and
credential handling. Its 17 offline tests passed, including UUID retention and
GET-only resume. The concurrently modified `.agents` skill files were not edited.

Recorded completed tasks:

- Bastion: `e3b0ec3a-5646-4b1f-bb71-2d016b43ee55`
- Vector: `8679ef9f-e87a-4c66-acd8-0be9f6c564e1`
- Skimmer: `a92da776-d056-498d-8d5c-ba1080d5862d`

All three downloads are complete. Do not resubmit these jobs or resume into
their existing output files.

The offline render/report script is
`artifacts/model-jobs/vehicle-tools/inspect_model.py` and accepts the asset name
after Blender's `--` separator. It does not modify the source GLB.
