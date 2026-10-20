---
name: elevenlabs
description: Generate sound effects and looping ambience with the ElevenLabs API, save audio and provenance in a repository, and configure an encrypted repository-local API key. Use for ElevenLabs sound effects, not speech or music generation.
---

# ElevenLabs sound effects

Use the bundled `scripts/elevenlabs.py` with Python 3.10+ (standard library only).
Resolve the script relative to this skill, and pass the target Git repository with
`--repo`. Do not assume the shell is in the intended repository.

## Credentials

Run `status --repo <repo>` to check configuration without displaying the key or
making an API request. Prefer the existing configuration; never read secret files
into tool output or ask the user to paste a key into chat.

For first setup, have the user run this in their own interactive terminal:

```powershell
python <skill-directory>/scripts/elevenlabs.py configure --repo <repo>
```

The hidden prompt writes `.secrets/elevenlabs.dpapi`, encrypted with Windows DPAPI
for the current Windows user. Add `/.secrets/` to the repository's `.gitignore`
before setup; the helper refuses a tracked or non-ignored secret path. The file is
local to that Windows account/machine; configure again on other machines. DPAPI
protects storage, not a compromised process running as the same user. Never bundle
`.secrets` into browser builds, artifacts or deployments. Never force-add it.
Re-running configure replaces the stored key after a successful encryption.

For CI or non-Windows hosts, inject `ELEVENLABS_API_KEY` through the environment's
secret manager. This takes precedence over the encrypted file. Do not put real
keys in command arguments, committed config, plaintext dotenv files or logs.

## Generate

A user request to generate a described effect authorizes that generation. Skill
setup alone does not authorize paid example generations. Keep batches within the
requested count. Use an explicit duration when known (0.5–30 seconds); omit it
for the provider to choose. Add `--loop` for looping ambience.

```powershell
python <skill-directory>/scripts/elevenlabs.py generate --repo <repo> --prompt "One soft wooden gate latch click, close and dry, no speech or music" --duration 1.5 --output assets/runtime/audio/gate-latch.mp3
```

The helper uses `eleven_text_to_sound_v2`, MP3 44.1 kHz / 128 kbps, and optionally
`--prompt-influence` (0–1). Output paths must stay inside the repository and cannot
overwrite an existing audio file or provenance sidecar. Use a new filename for
variants. The `.mp3.provenance.json` records the prompt, settings, UTC time, audio
hash, request ID and provider cost header when available, without credentials.
License/plan entitlement is recorded as unverified; establish the actual account
rights and attribution requirements before release, rather than inventing a license.

Read repository asset/audio conventions before selecting a destination. Generating
an asset does not automatically integrate it into gameplay. When requested, follow
the repository's engine workflow for integration. Listen to the output when audio
playback is available, assess clipping, unwanted voices/music and loop seams, and
report the actual verification scope. Link or embed the saved audio for the user.

Requests have a timeout and no automatic retries. A timeout or transport failure
may have consumed credits; report that uncertainty before deciding on a retry.
Authentication, quota and validation errors require fixing their cause first.

API schema checked 2026-10-11 against the official
[sound-effects reference](https://elevenlabs.io/docs/api-reference/text-to-sound-effects/convert).
Recheck that source when changing request fields or handling provider schema errors.
Run `python -m unittest discover -s <skill-directory>/scripts -p "test_*.py"`
after changing the helper. Tests use a fake transport and do not spend credits.
