# Asset production

## Skills

Repository copies:

- `.agents/skills/tripo/SKILL.md`: ImageGen reference → Tripo image-to-3D.
- `.agents/skills/elevenlabs/SKILL.md`: sound effects and looping ambience.
- `.agents/skills/imagegen/SKILL.md`: reference-image creation.
- `.agents/skills/engine-skill/SKILL.md`: WebEngine integration.

These were copied from Johan's central Codex skills on 2026-10-20. Provider
helpers use Python's standard library. ImageGen's explicit API fallback has
separate dependencies and authorization requirements; the built-in tool is preferred.
Skill availability does not guarantee that a cloud host provides ImageGen.

## Configure this computer

Run these commands yourself in an interactive terminal; both prompts hide input:

```powershell
python .agents/skills/tripo/scripts/tripo.py configure --repo .
python .agents/skills/elevenlabs/scripts/elevenlabs.py configure --repo .
```

Encrypted credentials remain in ignored `.secrets/`, bound to this Windows
account/machine. Existing keys in another project have not been copied.
Check readiness without revealing keys or contacting providers:

```text
python .agents/skills/tripo/scripts/tripo.py status --repo .
python .agents/skills/elevenlabs/scripts/elevenlabs.py status --repo .
python scripts/dev.py check-skills
```

For approved non-Windows runtime environments the helpers accept `TRIPO_API_KEY`
and `ELEVENLABS_API_KEY` from a secret manager. Do not put them into browser JS,
plain environment settings, command arguments, `.env` files, Git or chat.
Codex Cloud setup-only secrets cannot enable later agent-phase generation.
Generation is optional; builds/tests never require provider keys or credits.

## Locations and review

| Material | Location | Sync |
| --- | --- | --- |
| Approved optimized GLB/textures/audio + sanitized provenance | `assets/runtime/` | Commit to Git |
| ImageGen reference and prompt/hash | `artifacts/model-input-images/<asset>/` | Ignored working output |
| Tripo job ID/checkpoints/raw output | `artifacts/model-jobs/<asset>/` | Ignored; retain for resume |
| Source archives, renders and QA | `artifacts/` | Ignored; transfer intentionally if needed |
| Secrets | `.secrets/` | Never sync |

Do not lose paid job IDs during a machine switch. Finish/resume the job on the
same machine, or explicitly transfer its sanitized job metadata and reference
through an approved private artifact store. Do not restart an uncertain paid job.
Before promotion, retain sanitized provenance beside the runtime file and check
that it contains no signed download URLs, credentials or private machine paths.
Large source assets belong in an agreed artifact store; GitHub rejects ordinary
Git files over 100 MiB. Choose Git LFS deliberately if runtime assets grow.

## Initial direction

Use readable, realistic racing assets with consistent meter scale, complete
silhouettes and efficient browser geometry/textures. Keep concept placeholders
unbranded until a visual direction is chosen. Tripo references should show one
complete subject in a clear three-quarter studio view, with no text or clutter.
Inspect GLB orientation, dimensions, material maps and optimization before
integration. Inspect audio for clipping, unwanted speech/music and loop seams.
Record provider rights/entitlement evidence before release.

Setup is not authorization to buy sample generations. A future request describing
an asset authorizes the requested generation under the corresponding skill.
