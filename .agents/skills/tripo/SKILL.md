---
name: tripo
description: Create ImageGen references and Tripo 3D models, optionally split into parts in simple, balanced or detailed mode, with resumable tasks and encrypted local credentials. Use when the user requests Tripo generation, segmentation or API setup.
---

# Tripo 3D generation

Use `scripts/tripo.py` relative to this skill, with Python 3.10+ (standard library
only). Always pass the intended Git repository root with `--repo`.

## Required image-first workflow

For each new model, first use the `imagegen` skill and the built-in ImageGen tool
to create a reference image. Then submit that saved image to Tripo image-to-model.
Do not use Tripo text-to-model or pass a text prompt for model generation. Text
describes the ImageGen reference only; the Tripo helper accepts image inputs only.

Read the repository art direction before composing the reference. Show one
complete, isolated subject in a clear three-quarter view, with the entire
silhouette and important parts visible, neutral diffuse lighting, and a plain
background or actual transparency. Avoid cropped parts, scene clutter, text,
labels, multiple-view sheets, and perspective that hides the subject's structure.
For related models, keep palette, proportions and surface-detail scale consistent.
Use user-supplied references as inputs to ImageGen when preserving an identity.

Copy the generated image into the intended repository, for example
`artifacts/model-input-images/<asset>/reference-v1.png`, and inspect the saved
pixels before submission. Correct visible defects with ImageGen edits; do not
submit a failed reference. Retain the ImageGen prompt, reference sources, selected
image path and SHA256 alongside the job provenance. Keep references outside the
new model output directory, which the helper reserves at submission time.

Submit with `--image <saved-reference-path>`. If ImageGen is unavailable, report
the blocker; do not silently fall back to text-to-model. Follow the imagegen
skill's fallback rules. Updating or testing this skill does not authorize new
ImageGen references or paid Tripo sample generations. Resume existing model jobs
without regenerating their references or creating replacement tasks.

## Defaults and overrides

Apply these screenshot defaults unless the user overrides them:

| Setting | Default | API field |
| --- | --- | --- |
| Ultra Mesh Quality | On | `geometry_quality=detailed` |
| AI Complete | Off | `enable_image_autofix=false` (image input only) |
| Texture | On | `texture=true` |
| Texture Quality | 4K | `texture_quality=detailed` |
| Remove Lighting | Off | `delight=false` |
| PBR | On | `pbr=true` |
| Topology | Triangle | `quad=false` |
| Polycount | Maximum 20,000 triangles | `face_limit=20000` |

Keep every new model within a 20,000-triangle budget. The helper defaults to
20,000 triangle faces, accepts lower `--face-limit` values, and rejects higher
values before upload or submission. Quad output defaults to at most 10,000 faces
(20,000 triangles after triangulation). Inspect the delivered mesh's actual
triangle count before integration; a provider request limit is not output QA.

Geometry defaults to `v3.1-20260211`; texture is pinned to `v3.5-20260815` so
`delight=false` is honored (older texture models ignore that flag). Texture quality
is a provider tier, not a guarantee of every delivered map's pixel dimensions;
inspect the output before claiming exact 4K or an exact polygon count.
Boolean switches accept `--foo` and `--no-foo`; use `--help` for all overrides.
For bare geometry, `--no-texture` also disables PBR unless explicitly overridden;
explicit `--no-texture --pbr` is rejected because Tripo forces texturing with PBR.
Quads require `.fbx` output. AI Complete controls image input autofix;
do not confuse it with downstream mesh segmentation/part completion.

## Optional models in parts

Generate a whole model by default. When the user specifies "in parts", "separate
parts" or segmentation, generate/download the model first, then run `segment`
against that generation's `job.json`. If a suitable generated model already
exists, reuse its job instead of regenerating it or its reference.

Honor the requested **simple**, **balanced** or **detailed** parts mode. Default
to **balanced** when parts are requested without a mode. These are segmentation
granularity settings, independent of Ultra geometry, texture quality and face
count; a detailed parts request must not silently change those settings.

| Parts mode | Intended split |
| --- | --- |
| `simple` | A few broad components; useful when coarse separation is sufficient |
| `balanced` | Moderate semantic separation; default for requested parts |
| `detailed` | Finer components; inspect for excessive small fragments |

Example: "Create this cyberpunk car in parts, detailed mode" means the normal
ImageGen-to-model workflow followed by:

```powershell
python <skill-directory>/scripts/tripo.py segment --repo <repo> --source-job artifacts/model-jobs/car/job.json --mode detailed --output artifacts/model-jobs/car-parts/model.glb
```

The helper pins segmentation `v2.0-20260430` (provider Beta) and sends
`segmentation_granularity` to `POST /v3/mesh/segment`. Connectivity splitting
defaults on; use `--no-split-by-connectivity` when disconnected islands should
remain grouped. Do not pass the original image as segmentation `ref_image`:
that field expects segmentation guidance and makes the API ignore the mode.

Use this separate stage for mode-controlled parts. Generation's `generate_parts`
switch has no documented granularity setting and conflicts with textured/PBR
generation. The helper does not send it or silently turn off the existing
texture/PBR defaults. Inspect the segmented output's materials before claiming
texture preservation.

Keep the original model. The segmented result is one GLB containing parts, not
a promise of one file per part, closed surfaces, exact part count, usable pivots,
or a rig. Inspect separation, alignment and materials before integration.
For cars, check wheel/body separation and axle pivots explicitly. If individual
files are requested, export them after inspection while preserving assembly
transforms. Part completion, rigging and repeated segmentation are additional
work, not automatic follow-up operations.

A request to create a model in parts authorizes the generation and one requested
segmentation stage. Mention the extra paid stage when executing that request.
This skill update authorizes no paid calls. A different mode requires a new
segmentation task; do not automatically retry paid quality experiments.

Each stage has its own reserved output directory, task ID, provenance and
`job.json`. `segment --dry-run` previews the segmentation request without a key
or API call. Resume the appropriate stage with the existing `resume` command;
it only polls/downloads that recorded task and never resubmits. After resuming
generation, start the requested segmentation only if it has not been submitted.
If a segmentation submission has an uncertain outcome, inspect provider history
before creating any replacement.

## Credentials

Run `status --repo <repo>` to check local configuration without printing secrets
or contacting the provider. Never read secret files into tool output or ask for
keys in chat. First setup requires the user's own interactive terminal:

```powershell
python <skill-directory>/scripts/tripo.py configure --repo <repo>
```

The hidden prompt atomically stores Windows DPAPI encrypted bytes at
`.secrets/tripo.dpapi`, tied to the current Windows login. Like ElevenLabs, the
helper refuses a tracked or non-ignored `.secrets` directory. Ensure `/.secrets/`
is in `.gitignore`. Never force-add or ship `.secrets`. DPAPI does not protect
against another process running as the same user. For CI/non-Windows inject
`TRIPO_API_KEY` through a secret manager; it takes precedence over the local file.
No plaintext dotenv files, command-line keys or logged Authorization headers.

## Generate and resume

A user request for a described model authorizes that generation. Installing or
testing the skill alone does not authorize paid samples. Keep the requested count.
Read repository art direction/provenance conventions before choosing inputs and
destinations. For Paws cats/props, also follow `pawmodels` for review, normalization
and integration; generation alone does not modify the runtime or canonical rig.

```powershell
python <skill-directory>/scripts/tripo.py generate --repo <repo> --image artifacts/model-input-images/pot/reference-v1.png --output artifacts/model-jobs/pot/model.glb
```

`--image` uploads one local PNG/JPEG (up to 20 MB). `--image-url` uses a direct
HTTPS image URL; use it only for the selected ImageGen reference when a local file
cannot be used, and retain a project copy for provenance. `--prompt` is unsupported.
`--dry-run` validates
settings and prints the request without a key, upload or paid API call.

The helper reserves a new output directory and writes `job.json` before submission,
then persists the task ID immediately. It never retries task creation automatically.
A transport failure during submission may still consume credits: inspect the provider
task history before resubmitting. Resume known tasks without creating another:

```powershell
python <skill-directory>/scripts/tripo.py resume --repo <repo> --job artifacts/model-jobs/pot/job.json
```

Polling is bounded (default 20 minutes, configurable `--wait-seconds`); failed,
cancelled or unknown tasks stop. Resume uses the same key as submission. Download
immediately on success because result URLs expire. Model downloads receive no API
Authorization header. A provenance sidecar records request, task ID, credits,
timestamps and SHA256; it omits signed URLs and credentials. Account rights remain
unverified until actual entitlement evidence is recorded. Inspect geometry, maps,
silhouette and scale in Blender or the target viewer and report actual QA scope.

Generation and segmentation schemas checked 2026-10-20:
[image generation](https://developers.tripo3d.ai/en/docs/generation-image-to-model),
[segmentation](https://developers.tripo3d.ai/en/docs/mesh-segment),
[upload](https://developers.tripo3d.ai/en/docs/files),
[task query](https://developers.tripo3d.ai/en/docs/task-query).
Recheck official schemas when extending fields or encountering schema errors.

Validate changes with `python -m unittest discover -s <skill-directory>/scripts`.
Tests use fake HTTP responses and spend no credits.
