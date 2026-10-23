import io
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import urllib.error
import credentials
import tripo


class FakeTransport:
    def __init__(self, replies):
        self.replies = iter(replies)
        self.requests = []

    def open(self, req, timeout):
        self.requests.append(req)
        result = next(self.replies)
        if isinstance(result, Exception):
            raise result
        return io.BytesIO(result)


class Tests(unittest.TestCase):
    @staticmethod
    def glb():
        chunk = b'{}  '
        return b'glTF'+struct.pack('<II', 2, 20+len(chunk))+struct.pack('<I4s', len(chunk), b'JSON')+chunk

    def source_job(self, root):
        source = root/'source'
        source.mkdir()
        (source/'model.glb').write_bytes(self.glb())
        path = source/'job.json'
        tripo.save_json(path, dict(task_id='task_source', status='success',
                                  output='source/model.glb', request={'pbr': True}))
        return path

    def segment_args(self, *extra):
        return ['segment', '--repo', '.', '--source-job', 'source/job.json',
                '--output', 'parts/model.glb', *extra]

    def test_segmentation_modes_use_v2_without_changing_generation_quality(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder).resolve()
            self.source_job(root)
            for mode in ('simple', 'balanced', 'detailed'):
                with self.subTest(mode=mode):
                    args = tripo.parser().parse_args(self.segment_args('--mode', mode))
                    payload, provenance = tripo.segmentation_settings(args, root)
                    self.assertEqual(payload, dict(model='v2.0-20260430', input='task_source',
                        segmentation_granularity=mode, split_by_connectivity=True))
                    self.assertEqual(provenance['source_sha256'],
                                     tripo.hashlib.sha256(self.glb()).hexdigest())
            args = tripo.parser().parse_args(self.segment_args('--no-split-by-connectivity'))
            payload, _ = tripo.segmentation_settings(args, root)
            self.assertEqual(payload['segmentation_granularity'], 'balanced')
            self.assertFalse(payload['split_by_connectivity'])

    def test_segmentation_rejects_unfinished_or_invalid_source_and_fbx(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder).resolve()
            path = self.source_job(root)
            original = json.loads(path.read_text())
            args = tripo.parser().parse_args(self.segment_args())
            for changes in ({'status': 'running'}, {'task_id': '../bad'},
                            {'endpoint': '/mesh/segment'}, {'output': '../escape.glb'}):
                with self.subTest(changes=changes):
                    tripo.save_json(path, dict(original, **changes))
                    with self.assertRaises(ValueError):
                        tripo.segmentation_settings(args, root)
            tripo.save_json(path, original)
            args.output = 'parts/model.fbx'
            with self.assertRaisesRegex(ValueError, 'requires .glb'):
                tripo.segmentation_settings(args, root)
            args.output = 'parts/model.glb'
            (root/'source/model.glb').unlink()
            with self.assertRaisesRegex(ValueError, 'downloaded source'):
                tripo.segmentation_settings(args, root)

    def test_segmentation_dry_run_needs_no_key_and_creates_no_job(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder).resolve()
            self.source_job(root)
            with patch.object(tripo, 'repo_root', return_value=root), \
                    patch.object(tripo, 'get_key') as key, patch.object(tripo, 'api') as api, \
                    patch('sys.argv', ['tripo.py', *self.segment_args('--dry-run')]), \
                    patch('sys.stdout', new_callable=io.StringIO) as stdout:
                self.assertEqual(tripo.main(), 0)
            self.assertEqual(json.loads(stdout.getvalue())['endpoint'], tripo.BASE+'/mesh/segment')
            key.assert_not_called()
            api.assert_not_called()
            self.assertFalse((root/'parts').exists())

    def test_segment_submits_once_preserves_source_and_records_provenance(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder).resolve()
            source = self.source_job(root)
            original = source.read_bytes()
            transport = FakeTransport([
                b'{"code":0,"data":{"task_id":"task_segment"}}',
                b'{"code":0,"data":{"status":"success","credits_consumed":40,'
                b'"output":{"model_url":"https://cdn.tripo3d.ai/parts.glb?signature=private"}}}',
                self.glb()])
            with patch.object(tripo, 'repo_root', return_value=root), \
                    patch.object(tripo, 'get_key', return_value='test-key'), \
                    patch.object(tripo.urllib.request, 'build_opener', return_value=transport), \
                    patch('sys.argv', ['tripo.py', *self.segment_args('--mode', 'detailed')]):
                self.assertEqual(tripo.main(), 0)
            self.assertEqual(source.read_bytes(), original)
            self.assertEqual((root/'source/model.glb').read_bytes(), self.glb())
            self.assertEqual((root/'parts/model.glb').read_bytes(), self.glb())
            self.assertEqual([r.get_method() for r in transport.requests], ['POST', 'GET', 'GET'])
            self.assertEqual(transport.requests[0].full_url, tripo.BASE+'/mesh/segment')
            self.assertEqual(json.loads(transport.requests[0].data)['input'], 'task_source')
            self.assertIsNone(transport.requests[-1].get_header('Authorization'))
            meta_text = (root/'parts/model.glb.provenance.json').read_text()
            meta = json.loads(meta_text)
            self.assertEqual(meta['request']['segmentation_granularity'], 'detailed')
            self.assertEqual(meta['endpoint'], '/mesh/segment')
            self.assertEqual(meta['task_id'], 'task_segment')
            self.assertEqual(Path(meta['source_job']), Path('source/job.json'))
            self.assertNotIn('test-key', meta_text)
            self.assertNotIn('signature', meta_text)

    def test_segment_uncertain_submission_cannot_be_automatically_resubmitted(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder).resolve()
            self.source_job(root)
            transport = FakeTransport([urllib.error.URLError('uncertain')])
            with patch.object(tripo, 'repo_root', return_value=root), \
                    patch.object(tripo, 'get_key', return_value='test-key'), \
                    patch.object(tripo.urllib.request, 'build_opener', return_value=transport), \
                    patch('sys.stderr', new_callable=io.StringIO):
                with patch('sys.argv', ['tripo.py', *self.segment_args()]):
                    self.assertEqual(tripo.main(), 1)
                    self.assertEqual(tripo.main(), 1)  # Reserved directory prevents another POST.
                with patch('sys.argv', ['tripo.py', 'resume', '--repo', '.',
                                       '--job', 'parts/job.json']):
                    self.assertEqual(tripo.main(), 1)
            job = json.loads((root/'parts/job.json').read_text())
            self.assertEqual(job['status'], 'submitting_unknown_if_interrupted')
            self.assertNotIn('task_id', job)
            self.assertEqual(len(transport.requests), 1)

    def test_segment_resume_only_downloads_recorded_task(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder).resolve()
            job_path = root/'job.json'
            tripo.save_json(job_path, dict(task_id='task_segment', output='model.glb',
                endpoint='/mesh/segment', request=dict(input='task_source',
                model=tripo.SEGMENT_MODEL, segmentation_granularity='simple')))
            transport = FakeTransport([
                b'{"code":0,"data":{"status":"success","output":{"model_url":"https://cdn.tripo3d.ai/parts.glb"}}}',
                self.glb()])
            tripo.wait_download(root, job_path, 'test-key', 10, transport)
            self.assertTrue(all(r.get_method() == 'GET' for r in transport.requests))
            self.assertEqual(transport.requests[0].full_url, tripo.BASE+'/tasks/task_segment')

    def test_missing_submission_id_preserves_uncertainty(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder)/'job.json'
            transport = FakeTransport([b'{"code":0,"data":{}}'])
            with self.assertRaisesRegex(ValueError, 'check provider history'):
                tripo.submit_job(path, {'request': {}}, '/mesh/segment', 'test', transport)
            self.assertEqual(json.loads(path.read_text())['status'], 'submitting_unknown_if_interrupted')
            self.assertEqual(len(transport.requests), 1)

    def test_settings_and_overrides(self):
        args = tripo.parser().parse_args(['generate', '--repo', '.', '--image', 'a.png', '--output', 'out/model.glb'])
        mode, payload = tripo.settings(args)
        self.assertEqual(mode, 'image-to-model')
        self.assertEqual(payload, dict(model='v3.1-20260211', geometry_quality='detailed',
            face_limit=20000, texture=True, pbr=True, quad=False, texture_quality='detailed',
            texture_version='v3.5-20260815', delight=False, enable_image_autofix=False))
        args.texture = False
        self.assertFalse(tripo.settings(args)[1]['pbr'])
        args.pbr = True
        with self.assertRaises(ValueError):
            tripo.settings(args)

    def test_quad_image_autofix(self):
        args = tripo.parser().parse_args(['generate', '--repo', '.', '--image', 'pot.png', '--output', 'out/model.fbx', '--quad'])
        self.assertTrue(tripo.settings(args)[1]['quad'])
        self.assertEqual(tripo.settings(args)[1]['face_limit'], 10000)
        args.ai_complete = True
        self.assertTrue(tripo.settings(args)[1]['enable_image_autofix'])

    def test_generation_triangle_budget(self):
        for quad, maximum in ((False, 20000), (True, 10000)):
            args = tripo.parser().parse_args(['generate', '--repo', '.', '--image', 'a.png',
                '--output', 'out/model.fbx' if quad else 'out/model.glb',
                *(['--quad'] if quad else [])])
            for count in (500, maximum - 1, maximum):
                with self.subTest(quad=quad, count=count):
                    args.face_limit = count
                    self.assertEqual(tripo.settings(args)[1]['face_limit'], count)
            for count in (499, maximum + 1, 50000):
                with self.subTest(quad=quad, count=count):
                    args.face_limit = count
                    with self.assertRaisesRegex(ValueError, 'maximum 20,000 triangles'):
                        tripo.settings(args)

    def test_over_budget_rejected_before_credentials_or_upload(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder).resolve()
            with patch.object(tripo, 'repo_root', return_value=root), \
                    patch.object(tripo, 'get_key') as key, patch.object(tripo, 'upload') as upload, \
                    patch.object(tripo, 'api') as api, \
                    patch('sys.argv', ['tripo.py', 'generate', '--repo', '.', '--image', 'a.png',
                        '--output', 'out/model.glb', '--face-limit', '20001']), \
                    patch('sys.stderr', new_callable=io.StringIO):
                self.assertEqual(tripo.main(), 1)
            key.assert_not_called()
            upload.assert_not_called()
            api.assert_not_called()
            self.assertFalse((root/'out').exists())

    def test_text_generation_rejected_before_submission(self):
        with patch('sys.stderr', new_callable=io.StringIO), self.assertRaises(SystemExit):
            tripo.parser().parse_args(['generate', '--repo', '.', '--prompt', 'pot', '--output', 'out/model.glb'])

    def test_resume_download_without_resubmitting_or_leaking_key(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            job = root/'job.json'
            tripo.save_json(job, dict(task_id='task_test', output='model.glb', request={'pbr': True}))
            chunk = b'{}  '
            glb = b'glTF'+struct.pack('<II', 2, 20+len(chunk))+struct.pack('<I4s', len(chunk), b'JSON')+chunk
            api_reply = json.dumps(dict(code=0, data=dict(status='success', credits_consumed=60,
                output=dict(model_url='https://cdn.tripo3d.ai/test.glb?signature=private')))).encode()
            transport = FakeTransport([api_reply, glb])
            tripo.wait_download(root, job, 'secret-key', 10, transport)
            self.assertEqual((root/'model.glb').read_bytes(), glb)
            self.assertEqual(transport.requests[0].get_header('Authorization'), 'Bearer secret-key')
            self.assertIsNone(transport.requests[1].get_header('Authorization'))
            self.assertTrue(all(req.get_method() == 'GET' for req in transport.requests))
            meta = (root/'model.glb.provenance.json').read_text()
            self.assertNotIn('secret-key', meta)
            self.assertNotIn('signature', meta)
            with self.assertRaises(ValueError):
                tripo.wait_download(root, job, 'secret-key', 10, transport)

    def test_submit_failure_no_retry_no_body(self):
        error = urllib.error.HTTPError(tripo.BASE, 401, 'hidden', {}, io.BytesIO(b'secret-key'))
        transport = FakeTransport([error])
        with self.assertRaisesRegex(ValueError, 'HTTP 401') as context:
            tripo.api('/generation/image-to-model', 'secret-key', {'input': 'file_test'}, transport)
        self.assertEqual(len(transport.requests), 1)
        self.assertNotIn('secret-key', str(context.exception))

    def test_failed_task_stops_and_preserves_job(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            job = root/'job.json'
            tripo.save_json(job, dict(task_id='task_test', output='model.glb'))
            transport = FakeTransport([b'{"code":0,"data":{"status":"failed"}}'])
            with self.assertRaises(ValueError):
                tripo.wait_download(root, job, 'test', 1, transport)
            self.assertEqual(json.loads(job.read_text())['status'], 'failed')
            self.assertFalse((root/'model.glb').exists())

    def test_path_and_secret_tracking(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder).resolve()
            subprocess.run(['git', 'init', '-q', str(root)], check=True)
            with self.assertRaises(ValueError):
                credentials.secret_path(root)
            (root/'.gitignore').write_text('/.secrets/\n')
            self.assertEqual(credentials.secret_path(root), root/'.secrets/tripo.dpapi')
            with self.assertRaises(ValueError):
                tripo.output_path(root, '../escape.glb')
            with self.assertRaises(ValueError):
                tripo.output_path(root, '.secrets/model.glb')
            (root/'.secrets').mkdir()
            (root/'.secrets/tripo.dpapi').write_bytes(b'not-a-key')
            subprocess.run(['git', '-C', str(root), 'add', '-f', '.secrets/tripo.dpapi'], check=True)
            with self.assertRaises(ValueError):
                credentials.secret_path(root)

    @unittest.skipUnless(os.name == 'nt', 'Windows DPAPI only')
    def test_real_dpapi_roundtrip(self):
        original = b'synthetic-test-value-only'
        encrypted = credentials.crypt(original)
        self.assertNotIn(original, encrypted)
        self.assertEqual(credentials.crypt(encrypted, decrypt=True), original)


if __name__ == '__main__':
    unittest.main()
