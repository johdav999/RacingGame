"""Direct Tripo generation and optional part segmentation with resumable jobs."""
import argparse
from datetime import datetime, timezone
import hashlib
import http.client
import json
import math
import re
import struct
import subprocess
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
import uuid
from credentials import configure, get_key, repo_root, secret_path, within
import os

BASE = 'https://openapi.tripo3d.ai/v3'
SEGMENT_MODEL = 'v2.0-20260430'


def valid_task_id(value):
    # Live v3 responses use UUIDs; documentation examples also use task_* IDs.
    return isinstance(value, str) and re.fullmatch(
        r'(?:task_[A-Za-z0-9_-]+|[0-9a-fA-F]{8}(?:-[0-9a-fA-F]{4}){3}-[0-9a-fA-F]{12})', value) is not None



class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None


def request(url, data=None, key=None, content_type='application/json', opener=None, limit=4*1024*1024):
    headers = {'Content-Type': content_type} if data is not None else {}
    if key:
        if not key.isascii() or any(ord(c) < 33 or ord(c) > 126 for c in key):
            raise ValueError('Invalid credential characters; configure again')
        headers['Authorization'] = 'Bearer ' + key
    req = urllib.request.Request(url, data=data, headers=headers)
    try:
        with (opener or urllib.request.build_opener(NoRedirect())).open(req, timeout=60) as response:
            result = response.read(limit + 1)
            if not result or len(result) > limit:
                raise ValueError('Response empty or too large')
            return result
    except urllib.error.HTTPError as error:
        raise ValueError(f'Tripo/download HTTP {error.code}; body suppressed; no automatic retry') from None
    except (urllib.error.URLError, TimeoutError, ConnectionError, http.client.HTTPException):
        raise ValueError('Transport failed; submission may have consumed credits. No automatic retry') from None


def api(path, key, payload=None, opener=None, content_type='application/json'):
    data = payload if isinstance(payload, bytes) else (json.dumps(payload).encode() if payload is not None else None)
    try:
        response = json.loads(request(BASE + path, data, key, content_type, opener))
        if response.get('code') != 0:
            code = response.get('code')
            raise ValueError(f'Tripo API error code {code if isinstance(code, int) else "unknown"}; body suppressed')
        if not isinstance(response.get('data'), dict):
            raise ValueError('Invalid API response data')
        return response['data']
    except (json.JSONDecodeError, AttributeError):
        raise ValueError('Invalid API JSON response') from None


def save_json(path, value):
    temporary = path.with_name(path.name + '.' + uuid.uuid4().hex + '.tmp')
    try:
        with temporary.open('x', encoding='utf-8') as stream:
            json.dump(value, stream, indent=2, ensure_ascii=False)
            stream.write('\n')
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def output_path(root, value):
    path = within(root, value)
    if any(p in {'.git', '.secrets', '.agents', '.codex'} for p in path.relative_to(root).parts):
        raise ValueError('Output cannot be in repository configuration directories')
    if path.suffix.lower() not in {'.glb', '.fbx'}:
        raise ValueError('Output must be .glb or .fbx')
    return path


def settings(args):
    texture = args.texture
    pbr = texture if args.pbr is None else args.pbr
    if pbr and not texture:
        raise ValueError('PBR requires texture; use --no-pbr with --no-texture')
    max_faces = 10000 if args.quad else 20000
    face_limit = max_faces if args.face_limit is None else args.face_limit
    if not 500 <= face_limit <= max_faces:
        raise ValueError(f'Face limit must be between 500 and {max_faces:,}; maximum 20,000 triangles')
    if args.quad != (args.output.lower().endswith('.fbx')):
        raise ValueError('Triangle output requires .glb; quad output requires .fbx')
    payload = dict(model=args.model, geometry_quality='detailed' if args.ultra else 'standard',
                   face_limit=face_limit, texture=texture, pbr=pbr, quad=args.quad)
    if texture:
        payload.update(texture_quality=args.texture_quality, texture_version=args.texture_version,
                       delight=args.remove_lighting)
        if args.texture_version != 'v3.5-20260815':
            raise ValueError('This helper requires v3.5 textures to honor Remove Lighting; extend after schema review')
    payload['enable_image_autofix'] = args.ai_complete
    mode = 'image-to-model'
    if args.image_url:
        parsed = urllib.parse.urlsplit(args.image_url)
        if parsed.scheme != 'https' or not parsed.hostname or parsed.username or parsed.password:
            raise ValueError('Image URL must be HTTPS without embedded credentials')
        payload['input'] = args.image_url
    for field in ('model_seed', 'texture_seed'):
        value = getattr(args, field)
        if value is not None:
            if not 0 <= value <= 2147483647:
                raise ValueError('Seeds must be between 0 and 2147483647')
            payload[field] = value
    return mode, payload


def segmentation_settings(args, root):
    if not args.output.lower().endswith('.glb'):
        raise ValueError('Segmented output requires .glb')
    source_path = within(root, args.source_job)
    source = json.loads(source_path.read_text(encoding='utf-8'))
    if not isinstance(source, dict):
        raise ValueError('Source job must be a JSON object')
    task_id = source.get('task_id', '')
    if not valid_task_id(task_id):
        raise ValueError('Source job has no valid task ID; resolve the original job first')
    if source.get('status') != 'success':
        raise ValueError('Source job must have succeeded; resume the original job first')
    if source.get('endpoint', '/generation/image-to-model') != '/generation/image-to-model':
        raise ValueError('Segmentation requires the original image-to-model job')
    source_output = output_path(root, source['output'])
    if source_output.parent != source_path.parent or not source_output.is_file():
        raise ValueError('Keep the downloaded source model beside its source job')
    payload = dict(model=SEGMENT_MODEL, input=task_id,
                   segmentation_granularity=args.mode,
                   split_by_connectivity=args.split_by_connectivity)
    provenance = dict(source_job=str(source_path.relative_to(root)),
                      source_output=str(source_output.relative_to(root)),
                      source_sha256=hashlib.sha256(source_output.read_bytes()).hexdigest())
    return payload, provenance


def submit_job(job_path, job, endpoint, key, opener=None):
    # Persist uncertainty before POST; never repeat a possibly accepted submission.
    job['endpoint'] = endpoint
    job['status'] = 'submitting_unknown_if_interrupted'
    save_json(job_path, job)
    task_id = api(endpoint, key, job['request'], opener).get('task_id')
    if not valid_task_id(task_id):
        raise ValueError('Submission returned no valid task ID; check provider history before resubmitting')
    job['task_id'] = task_id
    job['status'] = 'submitted'
    save_json(job_path, job)


def upload(path, key, opener=None):
    if path.suffix.lower() not in {'.png', '.jpg', '.jpeg'}:
        raise ValueError('Image must be PNG or JPEG')
    data = path.read_bytes()
    if not data or len(data) > 20*1024*1024:
        raise ValueError('Image must be nonempty and at most 20 MiB')
    png = data.startswith(b'\x89PNG\r\n\x1a\n')
    jpg = data.startswith(b'\xff\xd8\xff')
    if not (png or jpg):
        raise ValueError('Image signature must be PNG or JPEG')
    boundary = uuid.uuid4().hex
    kind = 'png' if png else 'jpeg'
    body = (f'--{boundary}\r\nContent-Disposition: form-data; name="file"; filename="input.{kind}"\r\n'
            f'Content-Type: image/{kind}\r\n\r\n').encode() + data + f'\r\n--{boundary}--\r\n'.encode()
    token = api('/files', key, body, opener, 'multipart/form-data; boundary=' + boundary).get('file_token')
    if not isinstance(token, str) or not re.fullmatch(r'file_[A-Za-z0-9_-]+', token):
        raise ValueError('Upload did not return a valid file token')
    return token, hashlib.sha256(data).hexdigest()


def wait_download(root, job_path, key, seconds, opener=None):
    job = json.loads(job_path.read_text(encoding='utf-8'))
    task_id = job.get('task_id', '')
    if not valid_task_id(task_id):
        raise ValueError('Job has no valid task ID; check provider history before resubmitting')
    output = output_path(root, job['output'])
    if output.parent != job_path.parent or output.exists():
        raise ValueError('Job output must be beside job.json and must not already exist')
    deadline = time.monotonic() + seconds
    while True:
        task = api('/tasks/' + task_id, key, opener=opener)
        status = task.get('status')
        job['status'] = status
        job['checked_at'] = datetime.now(timezone.utc).isoformat()
        save_json(job_path, job)
        print(f'Task {task_id}: {status}', flush=True)
        if status == 'success':
            break
        if status not in {'queued', 'running'}:
            raise ValueError('Task stopped without success; consult provider task history')
        if time.monotonic() >= deadline:
            raise ValueError('Polling limit reached; resume this job later (no new task needed)')
        time.sleep(min(5, max(0, deadline-time.monotonic())))
    url = task.get('output', {}).get('model_url')
    parsed = urllib.parse.urlsplit(url or '')
    if parsed.scheme != 'https' or not parsed.hostname or parsed.username or parsed.password:
        raise ValueError('Provider did not return a valid HTTPS model URL')
    # Deliberately omit Authorization on the provider storage/CDN download.
    data = request(url, opener=opener, limit=512*1024*1024)
    if output.suffix.lower() == '.glb':
        if len(data) < 20 or data[:4] != b'glTF' or struct.unpack_from('<II', data, 4) != (2, len(data)):
            raise ValueError('Download is not a valid GLB v2 container')
    elif not (data.startswith(b'Kaydara FBX Binary') or b'FBXHeaderExtension' in data[:4096]):
        raise ValueError('Download is not an FBX file')
    provenance = dict(job, provider='Tripo3D', api_base=BASE,
                      sha256=hashlib.sha256(data).hexdigest(), bytes=len(data),
                      credits_consumed=task.get('credits_consumed'),
                      completed_at=task.get('completed_at'),
                      license_status='Unverified: record account entitlement and applicable rights before release')
    sidecar = output.with_suffix(output.suffix + '.provenance.json')
    if sidecar.exists():
        raise ValueError('Provenance already exists; refusing overwrite')
    with output.open('xb') as stream:
        stream.write(data)
    save_json(sidecar, provenance)
    print(f'Saved {output}\nProvenance: {sidecar}')


def parser():
    p = argparse.ArgumentParser(description=__doc__)
    commands = p.add_subparsers(dest='command', required=True)
    for name in ('configure', 'status', 'generate', 'segment', 'resume'):
        c = commands.add_parser(name)
        c.add_argument('--repo', required=True)
        if name in {'generate', 'segment', 'resume'}:
            c.add_argument('--wait-seconds', type=float, default=1200)
        if name == 'resume':
            c.add_argument('--job', required=True)
        if name == 'segment':
            c.add_argument('--source-job', required=True, help='Successful image-to-model job.json')
            c.add_argument('--output', required=True, help='GLB in a new directory; retains the source model')
            c.add_argument('--mode', choices=['simple', 'balanced', 'detailed'], default='balanced',
                           help='Part segmentation granularity (default: balanced)')
            c.add_argument('--split-by-connectivity', action=argparse.BooleanOptionalAction, default=True)
            c.add_argument('--dry-run', action='store_true')
        if name != 'generate':
            continue
        inputs = c.add_mutually_exclusive_group(required=True)
        inputs.add_argument('--image')
        inputs.add_argument('--image-url')
        c.add_argument('--output', required=True)
        c.add_argument('--dry-run', action='store_true')
        c.add_argument('--model', choices=['v3.1-20260211', 'v3.0-20250812'], default='v3.1-20260211')
        c.add_argument('--texture-version', default='v3.5-20260815')
        c.add_argument('--texture-quality', choices=['standard', 'detailed', 'extreme', 'fast'], default='detailed')
        c.add_argument('--face-limit', type=int,
                       help='Face budget: 500–20,000 triangles or 500–10,000 quads (default: topology maximum)')
        for flag, default in [('ultra', True), ('ai-complete', False), ('texture', True),
                              ('remove-lighting', False), ('pbr', None), ('quad', False)]:
            c.add_argument('--'+flag, action=argparse.BooleanOptionalAction, default=default)
        c.add_argument('--model-seed', type=int)
        c.add_argument('--texture-seed', type=int)
    return p


def main():
    args = parser().parse_args()
    try:
        root = repo_root(args.repo)
        if args.command == 'configure':
            configure(root)
        elif args.command == 'status':
            if os.environ.get('TRIPO_API_KEY', '').strip() or secret_path(root).is_file():
                get_key(root)
                print('Key configured and readable; provider validation not performed.')
            else:
                print('No Tripo API key configured.')
        else:
            if not math.isfinite(args.wait_seconds) or args.wait_seconds <= 0:
                raise ValueError('Wait seconds must be finite and positive')
            if args.command == 'resume':
                wait_download(root, within(root, args.job), get_key(root), args.wait_seconds)
            else:
                if args.command == 'segment':
                    payload, source = segmentation_settings(args, root)
                    endpoint = '/mesh/segment'
                else:
                    mode, payload = settings(args)
                    endpoint = '/generation/' + mode
                    source = {}
                output = output_path(root, args.output)
                image = None
                if args.command == 'generate' and args.image:
                    image = (root / args.image).resolve(strict=True)
                    if image.suffix.lower() not in {'.png', '.jpg', '.jpeg'} or image.stat().st_size > 20*1024*1024:
                        raise ValueError('Image must be PNG/JPEG at most 20 MiB')
                if args.dry_run:
                    print(json.dumps(dict(endpoint=BASE+endpoint, request=payload,
                                          local_image=str(image) if image else None), indent=2))
                    return 0
                key = get_key(root)
                output.parent.mkdir(parents=True, exist_ok=False)
                job_path = output.parent / 'job.json'
                job = dict(output=str(output.relative_to(root)), request=payload, endpoint=endpoint, **source,
                           submitted_at=datetime.now(timezone.utc).isoformat(), status='not_submitted')
                save_json(job_path, job)
                if image:
                    payload['input'], job['image_sha256'] = upload(image, key)
                submit_job(job_path, job, endpoint, key)
                wait_download(root, job_path, key, args.wait_seconds)
    except (ValueError, OSError, KeyError, TypeError, subprocess.SubprocessError) as error:
        message = str(error) if isinstance(error, ValueError) else 'Local I/O or malformed job/response; details suppressed'
        print('Error: '+message, file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
