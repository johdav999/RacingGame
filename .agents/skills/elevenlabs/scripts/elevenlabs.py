"""Generate repository sound effects without exposing API credentials."""
import argparse
import ctypes
import getpass
import hashlib
import http.client
import json
import math
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from datetime import datetime, timezone
import urllib.error
import urllib.request
import warnings

ENDPOINT = 'https://api.elevenlabs.io/v1/sound-generation?output_format=mp3_44100_128'
SECRET = '.secrets/elevenlabs.dpapi'


def repo_root(value):
    root = Path(value).resolve(strict=True)
    result = subprocess.run(['git', '-C', str(root), 'rev-parse', '--show-toplevel'],
                            capture_output=True, text=True)
    if result.returncode or Path(result.stdout.strip()).resolve() != root:
        raise ValueError('--repo must be a Git repository root')
    return root


def within(root, relative):
    path = (root / relative).resolve()
    if not path.is_relative_to(root) or path == root:
        raise ValueError('Path must stay inside the repository')
    return path


def secret_path(root):
    path = within(root, SECRET)
    tracked = subprocess.run(['git', '-C', str(root), 'ls-files', '--', '.secrets'],
                             capture_output=True, text=True, check=True)
    ignored = subprocess.run(['git', '-C', str(root), 'check-ignore', '-q', '--', SECRET],
                             capture_output=True)
    if tracked.stdout.strip() or ignored.returncode != 0:
        raise ValueError('Add /.secrets/ to .gitignore and ensure no .secrets files are tracked')
    return path


def crypt(data, decrypt=False):
    if os.name != 'nt':
        raise ValueError('Encrypted file storage requires Windows; use ELEVENLABS_API_KEY here')
    from ctypes import wintypes

    class Blob(ctypes.Structure):
        _fields_ = [('size', wintypes.DWORD), ('data', ctypes.POINTER(ctypes.c_ubyte))]

    buffer = (ctypes.c_ubyte * len(data)).from_buffer_copy(data)
    source = Blob(len(data), buffer)
    target = Blob()
    library = ctypes.WinDLL('crypt32', use_last_error=True)
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    function = library.CryptUnprotectData if decrypt else library.CryptProtectData
    function.argtypes = [ctypes.POINTER(Blob), ctypes.c_void_p, ctypes.POINTER(Blob),
                         ctypes.c_void_p, ctypes.c_void_p, wintypes.DWORD, ctypes.POINTER(Blob)]
    function.restype = wintypes.BOOL
    kernel.LocalFree.argtypes = [ctypes.c_void_p]
    kernel.LocalFree.restype = ctypes.c_void_p
    if not function(ctypes.byref(source), None, None, None, None, 1, ctypes.byref(target)):
        raise ValueError('Windows credential protection failed; configure under this Windows login')
    try:
        return ctypes.string_at(target.data, target.size)
    finally:
        ctypes.memset(target.data, 0, target.size)
        kernel.LocalFree(target.data)
        ctypes.memset(buffer, 0, len(data))


def get_key(root):
    key = os.environ.get('ELEVENLABS_API_KEY', '').strip()
    if key:
        return key
    path = secret_path(root)
    if not path.is_file():
        raise ValueError('No key configured. Run configure in your own terminal first')
    return crypt(path.read_bytes(), decrypt=True).decode('utf-8')


def configure(root):
    path = secret_path(root)
    if os.name != 'nt':
        raise ValueError('Use ELEVENLABS_API_KEY on non-Windows hosts')
    if not sys.stdin.isatty():
        raise ValueError('Run configure in your own interactive terminal; never pass a key in chat')
    with warnings.catch_warnings():
        warnings.simplefilter('error', getpass.GetPassWarning)
        key = getpass.getpass('ElevenLabs API key (hidden): ').strip()
    if not key or any(c.isspace() for c in key):
        raise ValueError('Key must be nonempty and contain no whitespace')
    encrypted = crypt(key.encode('utf-8'))
    path.parent.mkdir(parents=True, exist_ok=True)
    temp = None
    try:
        with tempfile.NamedTemporaryFile(dir=path.parent, delete=False) as stream:
            temp = Path(stream.name)
            stream.write(encrypted)
        os.replace(temp, path)
    finally:
        if temp is not None:
            temp.unlink(missing_ok=True)
    print('Encrypted key stored; Git ignore verified. No API call made.')


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None


def generate(root, args, opener=None):
    if not args.prompt.strip():
        raise ValueError('Prompt cannot be empty')
    if args.duration is not None and not (math.isfinite(args.duration) and 0.5 <= args.duration <= 30):
        raise ValueError('Duration must be 0.5–30 seconds')
    if not math.isfinite(args.prompt_influence) or not 0 <= args.prompt_influence <= 1:
        raise ValueError('Prompt influence must be 0–1')
    output = within(root, args.output)
    relative = output.relative_to(root)
    if any(part in {'.git', '.secrets', '.agents', '.codex'} for part in relative.parts):
        raise ValueError('Choose an asset path outside repository configuration folders')
    if output.suffix.lower() != '.mp3':
        raise ValueError('Output must have an .mp3 extension')
    sidecar = output.with_suffix(output.suffix + '.provenance.json')
    if output.exists() or sidecar.exists():
        raise ValueError('Output or provenance already exists; choose a new filename')
    key = get_key(root)
    if not key.isascii() or not key or any(ord(c) < 33 or ord(c) > 126 for c in key):
        raise ValueError('Configured key has invalid characters; configure it again')
    payload = dict(text=args.prompt, loop=args.loop, model_id='eleven_text_to_sound_v2',
                   prompt_influence=args.prompt_influence)
    if args.duration is not None:
        payload['duration_seconds'] = args.duration
    request = urllib.request.Request(ENDPOINT, data=json.dumps(payload).encode('utf-8'),
                                     headers={'xi-api-key': key, 'Content-Type': 'application/json',
                                              'Accept': 'audio/mpeg'}, method='POST')
    opener = opener or urllib.request.build_opener(NoRedirect())
    # Reserve both names before spending credits; never overwrite existing artifacts.
    output.parent.mkdir(parents=True, exist_ok=True)
    audio_created = metadata_created = audio_complete = False
    try:
        with output.open('xb') as audio:
            audio_created = True
            with sidecar.open('x', encoding='utf-8') as metadata:
                metadata_created = True
                try:
                    with opener.open(request, timeout=180) as response:
                        content_type = response.headers.get('Content-Type', '').split(';')[0].lower()
                        if content_type not in {'audio/mpeg', 'audio/mp3', 'application/octet-stream'}:
                            raise ValueError('Provider returned unexpected content type; no audio accepted')
                        data = response.read(32 * 1024 * 1024 + 1)
                        if not data or len(data) > 32 * 1024 * 1024:
                            raise ValueError('Provider audio is empty or exceeds 32 MiB')
                        if not (data.startswith(b'ID3') or (len(data) >= 2 and data[0] == 255 and data[1] & 224 == 224)):
                            raise ValueError('Provider response has no MP3 signature')
                        provenance = dict(provider='ElevenLabs', endpoint=ENDPOINT, request=payload,
                                          generated_at=datetime.now(timezone.utc).isoformat(),
                                          sha256=hashlib.sha256(data).hexdigest(),
                                          request_id=response.headers.get('request-id'),
                                          character_cost=response.headers.get('character-cost'),
                                          license_status='Unverified: record account plan and applicable rights before release')
                        audio.write(data)
                        audio.flush()
                        audio_complete = True
                        json.dump(provenance, metadata, indent=2, ensure_ascii=False)
                        metadata.write('\n')
                except urllib.error.HTTPError as error:
                    raise ValueError(f'ElevenLabs HTTP {error.code}; response body suppressed. No automatic retry') from None
                except (urllib.error.URLError, TimeoutError, ConnectionError, http.client.HTTPException):
                    raise ValueError('Transport failed; generation may have consumed credits. No automatic retry') from None
    except Exception:
        if not audio_complete:
            if audio_created:
                output.unlink(missing_ok=True)
            if metadata_created:
                sidecar.unlink(missing_ok=True)
        # Preserve a completed paid download if writing its provenance failed.
        raise
    print(f'Saved {output}\nProvenance: {sidecar}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='command', required=True)
    for name in ('configure', 'status', 'generate'):
        command = commands.add_parser(name)
        command.add_argument('--repo', required=True)
        if name == 'generate':
            command.add_argument('--prompt', required=True)
            command.add_argument('--output', required=True)
            command.add_argument('--duration', type=float)
            command.add_argument('--loop', action='store_true')
            command.add_argument('--prompt-influence', type=float, default=0.3)
    args = parser.parse_args()
    try:
        root = repo_root(args.repo)
        if args.command == 'configure':
            configure(root)
        elif args.command == 'status':
            if os.environ.get('ELEVENLABS_API_KEY', '').strip():
                print('Environment key configured (not validated with provider).')
            elif secret_path(root).exists():
                get_key(root)
                print('Encrypted key readable (not validated with provider).')
            else:
                print('No API key configured.')
        else:
            generate(root, args)
    except (ValueError, OSError, subprocess.SubprocessError, getpass.GetPassWarning) as error:
        # Do not dump request objects, provider bodies or credential contents.
        message = str(error) if isinstance(error, ValueError) else 'Local I/O or credential operation failed'
        print(f'Error: {message}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
