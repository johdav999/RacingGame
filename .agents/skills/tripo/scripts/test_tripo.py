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
    def test_settings_and_overrides(self):
        args = tripo.parser().parse_args(['generate', '--repo', '.', '--image', 'a.png', '--output', 'out/model.glb'])
        mode, payload = tripo.settings(args)
        self.assertEqual(mode, 'image-to-model')
        self.assertEqual(payload, dict(model='v3.1-20260211', geometry_quality='detailed',
            face_limit=50000, texture=True, pbr=True, quad=False, texture_quality='detailed',
            texture_version='v3.5-20260815', delight=False, enable_image_autofix=False))
        args.texture = False
        self.assertFalse(tripo.settings(args)[1]['pbr'])
        args.pbr = True
        with self.assertRaises(ValueError):
            tripo.settings(args)

    def test_quad_image_autofix(self):
        args = tripo.parser().parse_args(['generate', '--repo', '.', '--image', 'pot.png', '--output', 'out/model.fbx', '--quad'])
        self.assertTrue(tripo.settings(args)[1]['quad'])
        args.ai_complete = True
        self.assertTrue(tripo.settings(args)[1]['enable_image_autofix'])

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
